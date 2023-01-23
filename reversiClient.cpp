#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <limits>


using namespace std;

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

int main(int argc, char** argv) {   // "wmove" "board x" "player x"
  if(argc <= 1) {
    std::cout << "You must provide an IP address (1) and a port (2).";
    return -1;
  }

  char message[1000];
  char buffer[1024];
  int clientSocket;
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
  string msgStr;
  bool notify = false;

  Board board;

  int player = 1;
  std::string response = "";

  int boardResponse = 0;

  char playerSign = 'X';

  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(argv[2]));
  serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
  addr_size = sizeof(serverAddr);
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

  for(;;) {
    if(recv(clientSocket, buffer, 1024, 0) < 0) {
      if(!notify) cout << "No server connection or read error. Waiting for server..." << endl;
      notify = true;
      connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
    } else {
      notify = false;
      if(strstr(buffer, "player") != NULL)  {   // player x or player o
        playerSign = buffer[7];

        board.display();
        cout << "Connection established and a new game is starting! You're player " << playerSign << ". Wait for your turn." << endl;
      } else if(strstr(buffer, "proceed") != NULL || strstr(buffer, "wmove") != NULL) {
        if(strstr(buffer, "wmove") != NULL)
          cout << "Provide a correct value (0-8): ";
        else
          cout << "It's your turn: ";

        cin.clear();

        string joined;
        int x, y;
        while (!(cin >> y >> x) || x < 0 || x > 7 || y < 0 || y > 7) {
          cin.clear();
          cin.ignore(numeric_limits<streamsize>::max(), '\n');
          cout << "Invalid input, try again: ";
        }

        joined = to_string(x) + " " + to_string(y);
        strcpy(message, joined.c_str());


        if(strstr(message, "exit") != NULL) {
          printf("Exiting\n");
          break;
        }

        if(send(clientSocket, message, strlen(message)+sizeof(char)*8, 0) < 0) {
          printf("Send failed\n");
        }

        memset(&message, 0, sizeof (message));  // set every bit of message to 0
      }
            else if(strstr(buffer, "leave") != NULL) {
        printf("The game has ended.\n");
        break;
      }
            else if(strstr(buffer, "board") != NULL) {   // receives something like "board 2" from server then handles it
        char xMove = buffer[6];
        char yMove = buffer[7];
        char sign;
        player == 1 ? player = 2 : player = 1;
        board.placePiece((xMove - '0'), (yMove - '0'));
        board.display();
      } else {
        cout << "Server message: \"" << buffer << "\"" << endl;
      }
    }
  }

  close(clientSocket);
  return 0;
}
