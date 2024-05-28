//
//  stats.h
//
//  Author: David Meisner (meisner@umich.edu)
//

#ifndef STATS_H
#define STATS_H

#include "loader.h"
#include "config.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>


//#define N_HISTOGRAM_BINS 10000
//#define MIN_HISTOGRAM_VALUE 10e-6
//#define MAX_HISTOGRAM_VALUE 10

extern volatile int should_terminate;

struct config;

struct timeval start_time;

//A single statistic
struct stat {
  double s0;
  double s1;
  double s2;
  double min;
  double max;
  long fulls[1000];
  long thousands[1000];
  long millis[50010];
  long micros[10000];
};

/* 
 * Sample raw data
 * The size of array is 500k, thus the longest 
 * statistic interval with 1% sample rate is 
 * 1000sec when rps equals 50k/sec.
 */
#define RAW_SAMPLE_COUNT 500000
#define RAW_SAMPLE_INTERVAL 100
struct raw_stat {
  int count;
  int curr;
  double sendtime[RAW_SAMPLE_COUNT];
  float latency[RAW_SAMPLE_COUNT];
};

struct memcached_stats {
  int requests;
  int ops;
  int gets;
  int multigets;
  int sets;
  int hits;
  int misses;
  int incrs;
  int adds;
  int replaces;
  int deletes;
  struct stat response_time;
  struct stat get_response_time;
  struct stat set_response_time;
  struct stat get_size;
  struct timeval last_time;
  struct raw_stat raw_time;
};

extern pthread_mutex_t stats_lock;
//For now, all statistics are handled by this global struct
struct memcached_stats global_stats;
double findQuantile(struct stat* stat, double quantile);
void printGlobalStats();
void checkExit(struct config* config);
void addSample(struct stat* stat, float value,
  struct raw_stat* raw_time, struct request* request);
double getAvg(struct stat* stat);
double getStdDev(struct stat* stat);
void statsLoop(struct config* config);


#endif

