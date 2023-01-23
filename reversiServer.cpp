#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<fcntl.h> // for open
#include<unistd.h> // for close
#include<pthread.h>
#include<semaphore.h>
#include<iostream>

#define PORT 6969

using namespace std;

sem_t sem1, sem2;
char client_message[2000];
char buffer[1024];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;



class Board {

private:
  char board[8][8];
  char player;

public:
  // Initialize the board with empty tiles and starting player
  Board() {
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        board[i][j] = ' ';
      }
    }
    board[3][3] = 'X';
    board[4][4] = 'X';
    board[3][4] = 'O';
    board[4][3] = 'O';
    player = 'X';
  }

  char getPlayer() {
    return player;
  }

  // Function to display the current state of the board
  void display() {
    cout << endl << "    0   1   2   3   4   5   6   7   " << endl;
    for (int r = 0; r < 8; ++r) {
      cout << r << " ";
      cout << "| ";

      for (int c = 0; c < 8; ++c) {
        cout << board[r][c] << " | ";
      }

      cout << endl << "   -------------------------------" << endl;
    }
    cout << "Player's: " << player << " turn" << endl;
  }

  // Function to check if a move is legal
  bool isLegalMove(int x, int y) {
    // First check if the selected tile is empty
    if (board[x][y] != ' ') {
      return false;
    }

    // Get the opponent's player
    char opponent = (player == 'X') ? 'O' : 'X';
    bool legal = false;

    // Check all 8 directions (horizontally, vertically and diagonally) from the selected tile
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        // Skip the current tile
        if (i == 0 && j == 0) {
          continue;
        }

        // Get the next tile in the current direction
        int a = x + i;
        int b = y + j;

        // Check if the next tile is within the board and contains the opponent's piece
        if (a >= 0 && a < 8 && b >= 0 && b < 8 && board[a][b] == opponent) {
          // Keep moving in the current direction while encountering opponent's pieces
          a += i;
          b += j;
          while (a >= 0 && a < 8 && b >= 0 && b < 8 && board[a][b] == opponent) {
            a += i;
            b += j;
          }
          // Check if the next tile after the opponent's pieces is within the board and contains the player's piece
          if (a >= 0 && a < 8 && b >= 0 && b < 8 && board[a][b] == player) {
            legal = true;
          }
        }
      }
    }
    return legal;
  }

  // Function to place a piece on the board and flip opponent's pieces
  void placePiece(int x, int y) {
    if (isLegalMove(x, y)) {
      board[x][y] = player;
      flipPieces(x, y);
      player = (player == 'X') ? 'O' : 'X';
    } else {
      cout << "Move x: " << x << " y: " << y << " is invalid." << endl;
    }
  }

  // Function to flip the pieces
  void flipPieces(int x, int y) {
    char opponent = (player == 'X') ? 'O' : 'X';
    // Check all 8 directions (horizontally, vertically and diagonally) from the selected tile
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        // Skip the current tile
        if (i == 0 && j == 0) {
          continue;
        }
        int a = x + i;
        int b = y + j;
        // Check if the next tile is within the board and contains the opponent's piece
        if (a >= 0 && a < 8 && b >= 0 && b < 8 && board[a][b] == opponent) {
          // Keep moving in the current direction while encountering opponent's pieces
          a += i;
          b += j;
          while (a >= 0 && a < 8 && b >= 0 && b < 8 && board[a][b] == opponent) {
            a += i;
            b += j;
          }
          // Check if the next tile after the opponent's pieces is within the board and contains the player's piece
          if (a >= 0 && a < 8 && b >= 0 && b < 8 && board[a][b] == player) {
            // Start flipping the opponent's pieces in the current direction
            a = x + i;
            b = y + j;
            while (board[a][b] == opponent) {
              board[a][b] = player;
              a += i;
              b += j;
            }
          }
        }
      }
    }
  }

  // Function to check if the game is over and return the winner
  char checkWinner() {
    bool full = true;
    int x_count = 0;
    int o_count = 0;
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        if (board[i][j] == 'X') {
          x_count++;
        } else if (board[i][j] == 'O') {
          o_count++;
        } else {
          full = false;
        }
      }
    }
    if (full || !isMovePossible('X') || !isMovePossible('O')) {
      if (x_count > o_count) {
        return 'X';
      } else if (o_count > x_count) {
        return 'O';
      } else {
        return 'T'; // T stands for tie
      }
    }
    return 'N'; // N stands for not finished
  }

  // Function to check if a move is possible for a player
  bool isMovePossible(char player) {
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        if (isLegalMove(i, j)) {
          return true;
        }
      }
    }
    return false;
  }
};

class playerPair {

public:
    pthread_t player1;
    pthread_t player2;
    Board board;
    int sockets[2];
    int gameNumber = 0;
    int playerTurn = 0;
    int pcount = 0;
};

int currPair = 0;
playerPair pairs[1000];

void *player1(void* arg) {
    int gameId = currPair;
    int n;
    char* message;

    cout << "Game " << gameId << ": Player 1 joins." << endl;

    message = (char*)malloc(sizeof("player X"));
    strcpy(message, "player X");
    send(pairs[gameId].sockets[0], message, sizeof(message), 0);

    for(;;) {
        if(pairs[gameId].playerTurn == 1) {
            n = recv(pairs[gameId].sockets[0], client_message, 2000, 0);
            if(strstr(client_message, "leave") != NULL || n < 1) {
                //std::cout << "G" << std::to_string(gameId) << "P1 breaking." << std::endl;
                break;
            }
            std::cout << "G" << std::to_string(gameId) << "P1 message: " << client_message << std::endl;

            char* message = (char*)malloc(sizeof("Player 1 wins the game!"));

            usleep(100);

            string cMsgStr(client_message);
            if(cMsgStr.size() > 3) {
                usleep(100);
                message = (char*)malloc(sizeof("wmove"));
                strcpy(message, "wmove");
                send(pairs[gameId].sockets[0], message, sizeof(message), 0);
            }
            else if(stoi(cMsgStr) > 8 || stoi(cMsgStr) < 0) {
                usleep(100);
                message = (char*)malloc(sizeof("wmove"));
                strcpy(message, "wmove");
                send(pairs[gameId].sockets[0], message, sizeof(message), 0);
            }
            else {
                int x, y;
                x = cMsgStr[0] - 48;
                y = cMsgStr[2] - 48;
                bool isLegal = pairs[gameId].board.isLegalMove(x, y);
                char hasWon = pairs[gameId].board.checkWinner();
                pairs[gameId].board.placePiece(x, y);

                if(!isLegal) {
                    usleep(100);
                    message = (char*)malloc(sizeof("wmove"));
                    strcpy(message, "wmove");
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                }
                else if(isLegal) {
                    usleep(100);
                    string sMsg = "board " + to_string(x) + to_string(y);
                    message = (char*)malloc(sizeof(sMsg));
                    strcpy(message, sMsg.c_str());
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    pairs[gameId].playerTurn = 2;
                    usleep(100);
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);
                    usleep(100);
                    message = (char*)malloc(sizeof("proceed"));
                    strcpy(message, "proceed");
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);
                    memset(&client_message, 0, sizeof (client_message));
                }
                else if(hasWon == 'N') {
                    usleep(100);
                    string sMsg = "board " + to_string(x) + to_string(y);
                    message = (char*)malloc(sizeof(sMsg));
                    strcpy(message, sMsg.c_str());
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);
                    usleep(100);
                    string wMsg = "P1 wins!";
                    message = (char*)malloc(sizeof(wMsg));
                    strcpy(message, wMsg.c_str());
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);
                    usleep(100);
                    message = (char*)malloc(sizeof("leave"));
                    strcpy(message, "leave");
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);

                    break;
                }
            }
        }
    }

    pairs[gameId].playerTurn = 2;
    std::string msg = "leave";
    send(pairs[gameId].sockets[1], msg.c_str(), sizeof(msg.c_str()), 0);
    close(pairs[gameId].sockets[0]);
    close(pairs[gameId].sockets[1]);
    std::cout << "Player 1 of game " << std::to_string(gameId) << " leaves." << std::endl;
    pthread_exit(NULL);
}

void *player2(void* arg) {
    int gameId = currPair;
    cout << "Game " << gameId << ": Player 2 joins." << endl;
    int n;
    char* message = (char*)malloc(sizeof("proceed"));
    sleep(1);
    pairs[gameId].playerTurn = 1;

    message = (char*)malloc(sizeof("player O"));
    strcpy(message, "player O");
    send(pairs[gameId].sockets[1], message, sizeof(message), 0);

    std::string msg = "proceed";
    send(pairs[gameId].sockets[0], msg.c_str(), sizeof(msg.c_str()), 0);

    for(;;) {
        if(pairs[gameId].playerTurn == 2) {
            n = recv(pairs[gameId].sockets[1], client_message, 2000, 0);
            if(strstr(client_message, "leave") != NULL || n < 1) {
                //std::cout << "G" << std::to_string(gameId) << "P2 breaking." << std::endl;
                break;
            }
            std::cout << "G" << std::to_string(gameId) << "P2 message: " << client_message << std::endl;

            usleep(100);

            string cMsgStr(client_message);
            if(cMsgStr.size() > 3) {
                usleep(100);
                message = (char*)malloc(sizeof("wmove"));
                strcpy(message, "wmove");
                send(pairs[gameId].sockets[1], message, sizeof(message), 0);
            }
            else if(stoi(cMsgStr) > 8 || stoi(cMsgStr) < 0) {
                usleep(100);
                message = (char*)malloc(sizeof("wmove"));
                strcpy(message, "wmove");
                send(pairs[gameId].sockets[1], message, sizeof(message), 0);
            }
            else {
              int x, y;
              x = cMsgStr[0] - 48;
              y = cMsgStr[2] - 48;
              bool isLegal = pairs[gameId].board.isLegalMove(x, y);
              char hasWon = pairs[gameId].board.checkWinner();
              pairs[gameId].board.placePiece(x, y);

                if(!isLegal) {
                    usleep(100);
                    message = (char*)malloc(sizeof("wmove"));
                    strcpy(message, "wmove");
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);
                } else if(isLegal) {
                    usleep(100);
                    string sMsg = "board " + to_string(x) + to_string(y);
                    message = (char*)malloc(sizeof(sMsg));
                    strcpy(message, sMsg.c_str());

                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);
                    pairs[gameId].playerTurn = 1;
                    usleep(100);
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    usleep(100);
                    message = (char*)malloc(sizeof("proceed"));
                    strcpy(message, "proceed");
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    memset(&client_message, 0, sizeof (client_message));
                } else if(hasWon != 'N') {
                    usleep(100);
                    string sMsg = "board " + to_string(x) + to_string(y);
                    message = (char*)malloc(sizeof(sMsg));
                    strcpy(message, sMsg.c_str());
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);
                    usleep(100);
                    string wMsg = "P2 wins!";
                    message = (char*)malloc(sizeof(wMsg));
                    strcpy(message, wMsg.c_str());
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);
                    usleep(100);
                    message = (char*)malloc(sizeof("leave"));
                    strcpy(message, "leave");
                    send(pairs[gameId].sockets[0], message, sizeof(message), 0);
                    send(pairs[gameId].sockets[1], message, sizeof(message), 0);

                    break;
                }
            }
        }
    }

    pairs[gameId].playerTurn = 1;
    msg = "leave";
    send(pairs[gameId].sockets[0], msg.c_str(), sizeof(msg.c_str()), 0);
    close(pairs[gameId].sockets[0]);
    close(pairs[gameId].sockets[1]);
    std::cout << "Player 2 of game " << std::to_string(gameId) << " leaves." << std::endl;
    pthread_exit(NULL);
}

int main() {
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    int acceptedCycle = 0;

    //Create the socket.
    serverSocket = socket(PF_INET, SOCK_STREAM, 0);

    // Configure settings of the server address struct
    // Address family = Internet
    serverAddr.sin_family = AF_INET;

    //Set port number, using htons function to use proper byte order
    serverAddr.sin_port = htons(PORT);

    //Set IP address to localhost
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //Set all bits of the padding field to 0
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

    //Bind the address struct to the socket
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  //Listen on the socket
    if(listen(serverSocket,50)==0)
        printf("Server started.\n");
    else
        printf("Error\n");

    pthread_t thread_id;

    while(1) {
        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof(serverStorage);

        //newSocket = accept(serverSocket, (struct sockaddr*)&serverStorage, &addr_size);
        usleep(200);

        if(acceptedCycle == 2) {
            currPair++;
            //cout << "New currPair: " << currPair << endl;
            acceptedCycle = 0;
        }

        //cout << "Accepting..." << endl;
        pairs[currPair].sockets[pairs[currPair].pcount] = accept(serverSocket, (struct sockaddr*)&serverStorage, &addr_size);

        //cout << "Accepted something with pcount of " << pairs[currPair].pcount << endl;

        acceptedCycle++;

        if(pairs[currPair].pcount == 0) {
            pairs[currPair].gameNumber = currPair;

            if(pthread_create(&pairs[currPair].player1, NULL, player1, &pairs[currPair].sockets[pairs[currPair].pcount]) != 0)
                printf("Failed to create player 1 thread\n");
            else {
                pthread_detach(pairs[currPair].player1);
                //pthread_join(thread_id,NULL);
                pairs[currPair].pcount = 1;
            }
        }
        else
        if(pairs[currPair].pcount == 1) {
            pairs[currPair].gameNumber = currPair;

            if(pthread_create(&pairs[currPair].player2, NULL, player2, &pairs[currPair].sockets[pairs[currPair].pcount]) != 0)
                printf("Failed to create player 2 thread\n");
            else {
                pthread_detach(pairs[currPair].player2);
                //pthread_join(thread_id,NULL);
                pairs[currPair].pcount = 2;
            }
        }
    }

    close(serverSocket);
    return 0;
}