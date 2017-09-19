ESE 333 Real Time Operating Systems
Project 2

Author: Aditya Taday (109550833)

Software Information:

This software implements the producer-consumer problem. The contents of file1 will be copied to file2. The producer sends a signal after every chunk copied to shared buffer. The consumer sends a signal after every chunk put into the destination file. When the file copy is complete, the consumer send a termination signal to the main process. This triggers the main process to output the monitoring table. The monitoring table will have a series of 'P' and 'C' indicating the source of the signals.

Installation:

Compile using: gcc -o project2 ESE333_Project2_Aditya_Taday.c

Running the software:

To run a test, make sure you have a file with some content in it. Lets assume you have
a file named file1.txt in your folder. So to run a test, use:

# ./project2 file1.txt file2

Usage sample:

I've included the output of a sample run below :

avis@Avis:~/Downloads$ ls
CSE_215_HW_7.pdf                       output
cse_hw                                 readme.md
ESE333_Project2_Aditya_Taday.c         Screenshot from 2017-05-04 14-49-07.png
ESE333_Project2.pdf                    Screenshot from 2017-05-04 14-50-54.png
final-exam-2017-questions-FORMAT.docx  sem
inputfile                              test
avis@Avis:~/Downloads$ gcc -o project2 ESE333_Project2_Aditya_Taday.c 
avis@Avis:~/Downloads$ ./project2 readme.md readme2

Monitor Table Output
P C P C P C C
avis@Avis:~/Downloads$ cat readme2
avis@Avis:~/Downloads$ rm readme2

Contact Information:

Email: aditya.taday@stonybrook.edu

