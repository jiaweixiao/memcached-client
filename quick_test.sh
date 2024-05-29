memcached_path="."
memcached_conn=8
memcached_workers=10
memcached_preload_mb=512
qcd_thrupt=1000

memcached_args=""

memcached_args="src/loader \
  -a twitter_dataset/twitter_dataset_unscaled \
  -o twitter_dataset/twitter_dataset_90x \
  -s src/localhost \
  -w ${memcached_workers} -S 90 -D 16384 -j -T 2000"
echo "${memcached_path}/src/loader $memcached_args >& output.load"
${memcached_path}/src/loader $memcached_args >& output.load

# memcached_args="-a twitter_dataset/twitter_dataset_30x \
#     -s src/localhost \
#     -S 1 \
#     -j \
#     -D ${memcached_preload_mb} \
#     -T 100 \
#     -c ${memcached_conn} \
#     -w ${memcached_workers}"

# echo "${memcached_path}/src/loader $memcached_args >& output.load"
# ${memcached_path}/src/loader $memcached_args >& output.load

# memcached_args=" -a twitter_dataset/twitter_dataset_30x \
#     -s src/localhost \
#     -g 0.5 \
#     -T 60 \
#     -c ${memcached_conn} \
#     -w ${memcached_workers} \
#     -t 10 \
#     -r ${qcd_thrupt}"

# echo "${memcached_path}/src/loader $memcached_args >& output.run"
# ${memcached_path}/src/loader $memcached_args >& output.run