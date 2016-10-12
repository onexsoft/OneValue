# OneValue
A High Performance (About 1 million QPS) Persistent Key-Value Store Based on Redis Protocol

# Performance test
Environment:
Physical Machine (16  Intel(R) Xeon(R) CPU E5620  @ 2.40GHz 32G)

Config file:
<onevalue port="8221" thread_num="15" hash_value_max="240" work_dir="mydb">
  <db_node name="db1" hash_min="0" hash_max="19"></db_node>
  <db_node name="db2" hash_min="20" hash_max="39"></db_node>
  <db_node name="db3" hash_min="40" hash_max="59"></db_node>
  <db_node name="db4" hash_min="60" hash_max="79"></db_node>
  <db_node name="db5" hash_min="80" hash_max="99"></db_node>
  <db_node name="db6" hash_min="100" hash_max="119"></db_node>
  <db_node name="db7" hash_min="120" hash_max="139"></db_node>
  <db_node name="db8" hash_min="140" hash_max="159"></db_node>
  <db_node name="db9" hash_min="160" hash_max="179"></db_node>
  <db_node name="db10" hash_min="180" hash_max="199"></db_node>
  <db_node name="db11" hash_min="200" hash_max="219"></db_node>
  <db_node name="db12" hash_min="220" hash_max="239"></db_node>
</onevalue>


Test script:
for i in {1..5}
do
 nohup ./redis-benchmark -h [host] -p [port] -t get,set -r 1000000 -n 1000000 -q > ${i}.log 2>&1 &
done

Save to test.sh and exectuete.


cat *.log
SET: 60971.89 requests per second
GET: 84288.60 requests per second

SET: 60244.59 requests per second
GET: 83619.03 requests per second

SET: 60620.75 requests per second
GET: 80952.00 requests per second

SET: 60444.87 requests per second
GET: 81017.58 requests per second

SET: 60459.49 requests per second
GET: 87943.01 requests per second

Summary:
SET:30W+/s£¬GET:40W+/s

