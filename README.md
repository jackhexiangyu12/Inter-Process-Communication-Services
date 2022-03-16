Authors:
Gt username 1: Kwaldner3
Gt username 2: eirabor3

To test multiple files using --files, there's a text file for that, called "mult.txt"
Sometimes using small number of segments or smaller segment sizes can cause seg faults.
Increase the values if you get seg faults.

To start the process: run ./main --n_sms <num_segments> --sms_size <seg_size>
To test the client: run ./client --file <input_file> --state <SYNC | ASYNC>

