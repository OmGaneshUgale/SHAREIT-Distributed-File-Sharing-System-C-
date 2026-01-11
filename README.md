# 📡  SHAREIT-Distributed-File-Sharing-System-C-
SHAREIT is a distributed file sharing system implemented in C. It allows one system to send files to another system using a structured communication protocol. The system is composed of four processes (Brain, UserIO, Mouth, and Ear) that work together to manage sending, receiving, and reconstructing files.

## 🚀 Features
### ⚡ Fast – Efficient file transfer using metadata and bit vectors.

## 🎯 Simple – Easy to install and run.

## 🖥️ User-Friendly – Interactive interface for sending and receiving files.

## 🛠️ Installation

make

## ▶️ Usage

./system

## 📂 Project Structure
├── brain.c       # Controls all child processes (central controller)

├── ear.c         # Receives incoming messages and forwards to Brain

├── mouth.c       # Sends outgoing messages from Brain to target system

├── userio.c      # Handles user input/output

├── makefile      # Compilation rules

└── README.md     # Documentation

## ⚙️ Working

## 🧠 Brain

Acts as the central processing unit.
Controls all child processes.
Makes decisions based on received messages.

## 👂 Ear

Receives messages from external systems.
Forwards them to Brain.

## 👄 Mouth

Sends messages from Brain to the desired system.

## 🖱️ UserIO

Takes input from the user (file name, target system).
Sends commands/messages to Brain.
Displays received messages to the user.

## 🔄 File Transfer Protocol

User Input
User provides file name and target system via UserIO.
Message is passed to Brain.
File Segmentation
File size is calculated.
File is split into parts.
Each part is appended with metadata:

### OPTION | SENDER | RECEIVER | MSG NO | NO OF PARTS | ACTUAL MESSAGE

Message Exchange
Sender creates an information message and sends it to Receiver.
Receiver allocates space for incoming file and sends acknowledgment.
Sender transmits all parts, followed by an end message.
Receiver Processing
Receiver stores each part and updates a bit vector to track received parts.
On receiving the end message, Receiver sends acknowledgment with bit vector.
If parts are missing, Sender retransmits them.
Once all parts are received, Receiver reconstructs the file and saves it.
Entry is added to Till Received Messages.
Sender Finalization
When Sender receives acknowledgment with zero missing parts, it discards temporary data.
Entry is added to Till Sent Messages.

## 📊 Communication Flow

### Sender → Receiver

   SENDER SYSTEM  ----->---->  RECEIVER SYSTEM 


#### Messages:
- Start message
- Parts of message
- End message

### Receiver → Sender

   RECEIVER SYSTEM ----->---->   SENDER SYSTEM

#### Acknowledgments:
- Start message acknowledgment
- End message acknowledgment

## 🛠️ Technologies Used

C Programming Language

Processes & IPC (Inter-Process Communication)

File-based communication

Metadata-driven file segmentation

