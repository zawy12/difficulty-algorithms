#!/usr/bin/perl
use JSON;
 
# Copyright (c) 2018 Zpalmtree (Turtle coin) and Zawy
# MIT license
#
# Perl script to simulate a hash attack on Cryptonote testnet.
# CTRL-C to stop and stop attack simulation miner. "Stopping" attack means
# preventing your miner from being able to mine all the time. "Starting" means
# to allow miner continuous mining.

# Timestamp manipulation instructions (optional). You need 2 other nodes and 1 other miner on one of 
# them with 1 thread. The other nodes will have the valid chain. Get difficulties from them for analysis.
# Turn off automatic time updates on this node. Select the linux system of this node below. 
# Select 1 of the 4 recommended timestamp adjustments below and run until attacker finds 10 or more blocks. 
# Then stop attacking and turn automatic time update back on to fix system
# clock, then turn off again, then select a new bad timestamp adjustment.
 
############## BEGIN SETTINGS #############
$port = '31898';
$address='TRTLv1eJUEeV3uHbMpjbDefuhTdphfnDTij2skBJJMDi5EPcmEWpGTsJvRLpUA3F1T5wrjCjGS3GYCHyQiRxXaGR8FW8cjjv8Wi';

$T = 30; # coin's target solvetime
$AS = 10; # attack size as multiple of baseline hashrate

# optional timestamp manipulation settings
$use_timestamp_manipulation = 0; # if 1 then attacker will send sequence of bad timestamps. 
$bad_timestamp_size = $T;  # suggested settings to try: 1*$T, 10*T, -1*T, and -10*$T;
# non-ubuntu systems need root privilidges.
$linux_system = 0; # 0 = not linux, 1 = ubuntu, 2 = other GNU, 3  = BSD 
############ END SETTINGS #############
 
$attacker_threads = 4;
$dedicated_threads = 1;
 
$dedicated_miner_off_time = int((1-$attacker_threads/$dedicated_threads/$AS)*$T);
 
start_miner($dedicated_threads);
 
$last_height = get_height();
 
$SIG{INT}  = sub { $interrupted = 1; };
 
while ( 1 ) {
    sleep 1;
    if ($interrupted) {
        $interrupted = 0;
        print "Starting attack.\n";
        &start_miner($attacker_threads);
   
        while(!$interrupted) {
            sleep 1;  
            $height = get_height();
            if ($height != $last_height) {
                $last_height = $height;
                print "Attacker found $height.\n";
                if ( $use_timestamp_manipulation == 1 ) {
                    $secs=time; 
                    $secs+=$bad_timestamp_size; 
                    if    ( $linux_system == 1 ) { $temp =`sudo date -s '\@$secs'`;  }
                    elsif ( $linux_system == 2 ) { $temp =`date -s '\@$secs'`;  } 
                    elsif ( $linux_system == 3 ) { 
                       $temp =`date "$(date -r $secs +'%y%m%d%H%M.%S')"`; # untested
                    }
                    print "$secs\n";
                }
            }
        }
        $interrupted = 0;
        print "Attack Stopped.\n";
    }
 
    $height = get_height();
 
    if ($height != $last_height) {
        $last_height = $height;
        print "Dedicated miner found $height.\n";
        `pkill miner`;
        print "Miner sleeping for $dedicated_miner_off_time seconds.\n";
        sleep $dedicated_miner_off_time;
        print "Starting dedicated miner.\n";
        start_miner($dedicated_threads);
    }
}
 
sub get_height {
    my $height=`curl -s -X POST http://127.0.0.1:$port/json_rpc -d '{"params": {},"jsonrpc":"2.0","id":"test","method":"getblockcount"}' -H 'Content-Type: application/json'`;
    $decoded = decode_json($height);
    $height = $decoded->{'result'}{'count'};
    return $height;
}
 
sub start_miner {
    my $threads = $_[0];
    system("./miner --threads $threads --address $address --daemon-rpc-port $port &");
}
