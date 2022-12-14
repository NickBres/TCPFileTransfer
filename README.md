TCP CC
In this Ex you’ll write two program files: Sender.c and Receiver.c. The Sender will send a file and the Receiver will receive it and measure the time it took for his program to receive the file.
The file will be sent in two parts (first half and second half [Each half should be 50% of the file]) each half will be sent according to one of the CC algorithms you learned in the lectures.
You are obligated to send the file a few times (at least 5 times in each run) for creating data set.
You are obligated to run the program with a packet lost tool in the following levels: 0% lost, 10% lost, 15% lost, 20% lost.

The program steps: 
Sender.c:
  1. At first, you’ll read the file you’ve created.
  2. Create a TCP Connection between the sender and receiver.
  3. Send the first part of the file.
  4. Check for authentication (explaining below).
  5. Change the CC Algorithm.
  6. Send the second part.
  7. User decision:
    a. Send the file again? (For data gathering)
      i. If yes:
      ii. Change back the CC Algorithm.
      iii. Go back to step 3.
    b. Exit?
      i. If yes:
      ii. Send an exit message to the receiver.
      iii. Close the TCP connection.
Receiver.c:
  1. Create a TCP Connection between the sender and receiver. As known, it doesn’t create a whole new socket with TCP connection. Its just should establish connection with the socket that the sender created.
  2. Get a connection from the sender.
  3. Receive the first part of the file.
  4. Measure the time, it took to receive the first part.
  5. Save the time.
  6. Send back the authentication to the sender.
    i. Change the CC Algorithm.
  7. Receive the second part of the file.
  8. Measure the time it took to receive the second part.
  9. Save the time.
  10. If you get an exit message from the sender:
    i. Print out the times.
    ii. Calculate the average time for each part of the received files.
    iii. Print the average file.

