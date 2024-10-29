# Computer Networks Course Project TY CS A Group 5

## Design and Deploy TCP-based Multithreaded SMTP and POP3 Mail Client Server for Your Campus

### Overview

This project implements a TCP-based multithreaded POP3 mail server in C++. It allows users to authenticate and manage their emails using the POP3 protocol. The server supports various commands, including user authentication, email retrieval, and mailbox management.

### Video Demonstration

YouTube link to demonstration: [Watch the video](https://youtu.be/TVybTI8ME1s)

### Features

- **User Authentication**: Supports the `USER` command to authenticate users.
- **Email Retrieval**: Implements the `RPOP` command to retrieve emails.
- **Mailbox Management**: Supports commands like `STAT`, `LIST`, `DELE`, and `RSET` for mailbox operations.
- **Multithreading**: Handles multiple client connections simultaneously using threads.
- **Command Line Client**: A command line client to interact with the server.
- **Error Handling**: Provides appropriate error messages for invalid commands and authentication failures.

### Supported Commands

The following POP3 commands are implemented:

- `USER`: Authenticate a user.
- `RPOP`: Retrieve a message.
- `STAT`: Get mailbox statistics.
- `LIST`: List messages in the mailbox.
- `RETR`: Retrieve a specific message.
- `DELE`: Delete a message.
- `RSET`: Reset the mailbox state.
- `NOOP`: Ping the mail server.
- `LAST`: Retrieve the last accessed message.
- `TOP`: Retrieve headers of a message.
- `QUIT`: Exit the session.



