//
//  stats.c
//
//  Author: David Meisner (meisner@umich.edu)
//

#include "stats.h"
#include "loader.h"
#include <assert.h>
#include "worker.h"

pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

void addSample(struct stat* stat, float value,
  struct raw_stat* raw_time, struct request* request) {
  /* Value is in seconds */
  stat->s0 += 1.0;
  stat->s1 += value;
  stat->s2 += value*value;
  stat->min = fmin(stat->min,value);
  stat->max = fmax(stat->max,value);

  if(value < .001){
    int bin = (int)(value*10000000);
    stat->micros[bin] += 1;
  } else if( value < 5.0){
    int bin = value * 10000.0;
    assert(bin < 50001);
    stat->millis[bin] += 1;
  } else if (value < 999){
    int bin = (int)value;
    stat->fulls[bin] += 1;
  } else {
    int bin = (int)value/1000;
    if (bin > 999){
      bin = 999;
    }
    stat->fulls[bin] += 1;
  }

  if(request != NULL && raw_time != NULL && raw_time->count >= 0) {
    raw_time->count += 1;
    if ((raw_time->count % RAW_SAMPLE_INTERVAL) == 0 && 
         raw_time->curr < (RAW_SAMPLE_COUNT - 1)) {
        double sendtime = request->send_time.tv_sec +
                          1e-6 * request->send_time.tv_usec;
    	  raw_time->sendtime[raw_time->curr] = sendtime;
    	  raw_time->latency[raw_time->curr] = value;
	      raw_time->curr += 1;
    }
  }

}//End addAvgSample()

double getAvg(struct stat* stat) {
  return (stat->s1/stat->s0);
}//End getAvg()

double getStdDev(struct stat* stat) {
  return sqrt((stat->s0*stat->s2 - stat->s1*stat->s1)/(stat->s0*(stat->s0 - 1)));
}//End getStdDev()

void dumpLatencyRaw(struct config* config) {

  FILE* fileOut = fopen(config->dump_latency_file, "w");
  if (fileOut == NULL) {
     printf("dumpLatencyRaw: error opening file %s\n", config->dump_latency_file);
     return ;
  }
  int i;
  struct raw_stat* raw_time = &global_stats.raw_time;

  printf("Dump latency raw to file: %s\n", config->dump_latency_file);
  fprintf(fileOut, "ts_ms,latency_us\n");
  for (i = 0; i < raw_time->curr; i += 1) {
    fprintf(fileOut, "%f,%f\n", raw_time->sendtime[i]*1e3, raw_time->latency[i]*1e6);
  }
  fclose(fileOut);
}

void dumpLatencyHistogram(struct config* config){

  FILE* fileOut = fopen(config->dump_latency_file, "w");
  if (fileOut == NULL) {
     printf("dumpLatencyHistogram: error opening file %s\n", config->dump_latency_file);
     return ;
  }
  double i, q, qget, qset;

  printf("Dump latency histogram to file: %s\n", config->dump_latency_file);
  fprintf(fileOut, "Percentile,Value_us,Vget_us,Vset_us\n");
  for( i = .0; i <= 1.0; i += 0.0001) {
    q = findQuantile(&global_stats.response_time, i);
    qget = findQuantile(&global_stats.get_response_time, i);
    qset = findQuantile(&global_stats.set_response_time, i);
    fprintf(fileOut, "%f,%f,%f,%f\n", i, q*1e6, qget*1e6, qset*1e6);
  }
  fclose(fileOut);
}

//Should we exit because time has expired?
void checkExit(struct config* config) {

  int runTime = config->run_time;
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);
  double totalTime = currentTime.tv_sec - start_time.tv_sec + 1e-6*(currentTime.tv_sec - start_time.tv_sec);
  if(totalTime >= runTime && runTime >0) {
    printf("Ran for %f, exiting\n", totalTime);
    if (config->dump_latency_file != NULL) {
      if (str_endwith(config->dump_latency_file, ".hist"))
        dumpLatencyHistogram(config);
      else if (str_endwith(config->dump_latency_file, ".raw"))
        dumpLatencyRaw(config);
      else
        printf("checkExit(): dump latency fail.\n");
    }
    exit(0);
  }

}//End checkExit()

double findQuantile(struct stat* stat, double quantile) { 

  //Find the 95th-percentile
  long nTillQuantile = stat->s0 * quantile;
  long count = 0;
  int i;
  for( i = 0; i < 10000; i++) {
    count += stat->micros[i];
    if( count >= nTillQuantile ){
      double quantile = (i+1) * .0000001;
      return quantile;
    }
  }//End for i

  for( i = 0; i < 50000; i++) {
    count += stat->millis[i];
    if( count >= nTillQuantile ){
      double quantile = (i+1) * .0001;
      return quantile;
    }
  }//End for i
  //printf("count  %ld\n", count);

  for( i = 0; i < 1000; i++) {
    count += stat->fulls[i];
    if( count >= nTillQuantile ){
      double quantile = i+1;
      return quantile;
    }
  }//End for i
  return 1000;

}//End findQuantile()

void printGlobalStats(struct config* config) {

  pthread_mutex_lock(&stats_lock);
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);
  double timeDiff = currentTime.tv_sec - global_stats.last_time.tv_sec + 1e-6*(currentTime.tv_sec - global_stats.last_time.tv_sec);
  double rps = global_stats.requests/timeDiff;
  //double std = getStdDev(&global_stats.response_time);
  //double q90 = findQuantile(&global_stats.response_time, .90);
  //double q95 = findQuantile(&global_stats.response_time, .95);
  double q99 = findQuantile(&global_stats.response_time, .99);

  //printf("%10s,%8s,%16s, %8s,%11s,%10s,%13s,%10s,%10s,%10s,%12s,%10s,%10s,%11s,%14s\n", "timeDiff", "rps", "requests", "gets", "sets",  "hits", "misses", "avg_lat", "90th", "95th", "99th", "std", "min", "max", "avgGetSize");
  //printf("%10f, %9.1f,  %10d, %10d, %10d, %10d, %10d, %10f, %10f, %10f, %10f, %10f, %10f, %10f, %10f\n", 
		//timeDiff, rps, global_stats.requests, global_stats.gets, global_stats.sets, global_stats.hits, global_stats.misses,
		//1000*getAvg(&global_stats.response_time), 1000*q90, 1000*q95, 1000*q99, 1000*std, 1000*global_stats.response_time.min, 1000*global_stats.response_time.max, getAvg(&global_stats.get_size));
  //int i;
  //printf("Outstanding requests per worker:\n");
  //for(i=0; i<config->n_workers; i++){
  //  printf("%d ", config->workers[i]->n_requests);
  //} 
  //printf("\n");
  printf("%10.1f, %9.1f, %10d, %10d, %10.3f, %10.3f\n", 
		timeDiff, rps, global_stats.gets, global_stats.sets,
		1e6*getAvg(&global_stats.response_time), 1e6*q99);
  //Reset stats if do not dump latency histogram
  if(config->dump_latency_file == NULL && config->pre_load == 0) {
    memset(&global_stats, 0, sizeof(struct memcached_stats));
  } else {
    // Reset except response_time, get_response_time, set_response_time
    global_stats.requests = 0;
    global_stats.ops = 0;
    global_stats.gets = 0;
    global_stats.multigets = 0;
    global_stats.sets = 0;
    global_stats.hits = 0;
    global_stats.misses = 0;
    global_stats.incrs = 0;
    global_stats.adds = 0;
    global_stats.replaces = 0;
    global_stats.deletes = 0;
    memset(&(global_stats.get_size), 0, sizeof(struct stat));
  }
  global_stats.response_time.min = 1000000;
  global_stats.get_response_time.min = 1000000;
  global_stats.set_response_time.min = 1000000;
  global_stats.last_time = currentTime;

  checkExit(config);
  pthread_mutex_unlock(&stats_lock);

}//End printGlobalStats()


//Print out statistics every second
void statsLoop(struct config* config) {

  pthread_mutex_lock(&stats_lock);
  gettimeofday(&start_time, NULL);
  pthread_mutex_unlock(&stats_lock);

  sleep(2);
  printf("Stats:\n");
  printf("-------------------------\n");
  printf("%10s,%8s,%8s,%11s,%10s,%12s\n",
    "timeDiff_sec","rps","gets","sets","avg_us","99th_us");
  while(1) {
    printGlobalStats(config);
    sleep(config->stats_time);
  }//End while()


}//End statisticsLoop()

