# memcached-client
The source code is download from https://github.com/parsa-epfl/cloudsuite/tree/3.0/benchmarks/data-caching/client.
## Build
```bash
cd src
make
cd ..
src/loader -h
```
## Usage
Example:
1. Scaling the dataset (once for each scale)
   ```bash
   src/loader \
     -a twitter_dataset/twitter_dataset_unscaled \
     -o twitter_dataset/twitter_dataset_30x \
     -s src/local_servers.txt \
     -w 4 -S 30 -D 16384 -j -T 3
   ```
3. First preload data
  ```bash
  src/loader \
    -a twitter_dataset/twitter_dataset_30x \
    -s src/local_servers.txt \
    -S 1 \
    -j \
    -D <preloaded memory size in MB> \
    -T 60 \
    -c <num of connections> \
    -w <num of workers> \
  ```
3. Then run
  ```bash
  src/loader \
    -a twitter_dataset/twitter_dataset_<scale>x \
    -s src/local_servers.txt \
    -g <get ratio> \
    -T 60 \
    -c <num of connections> \
    -w <numb of workers> \
    -t <running time> \
    -r <rps>
  ```
