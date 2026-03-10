#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <emscripten/emscripten.h>

#define ROWS 5
#define COLS 10
#define MAX_SHOWS 3

// ────────────────────────────────────────────────
// Structures
// ────────────────────────────────────────────────
typedef struct {
    char title[50];
    int  seats[ROWS][COLS];   // 0 = available, 1 = booked
    float price;
} Show;

typedef struct {
    int  bookingID;
    char customerName[50];
    int  showIndex;
    int  numSeats;
    char seatPositions[200];
    float totalAmount;
} Booking;

// ────────────────────────────────────────────────
// Globals
// ────────────────────────────────────────────────
Show    shows[MAX_SHOWS];
Booking bookings[500];
int     bookingCount = 0;
int     nextBookingID = 1001;

// State machine
// 0=main menu  1=view show select  2=book show select
// 3=book num seats  4=book customer name  5=book seat entry
// 6=view booking id
static int state = 0;
static int selected_show  = -1;
static int seats_to_book  = 0;
static int seats_entered  = 0;
static char customer_name[50];
static char booked_seats_str[200];
static int  pending_seats[50][2];   // [n][0]=row [n][1]=col

// ────────────────────────────────────────────────
// Forward declarations
// ────────────────────────────────────────────────
void show_main_menu(void);
void display_seats(int idx);
void occupancy_report(void);

// ────────────────────────────────────────────────
// Startup
// ────────────────────────────────────────────────
int main() {
    srand((unsigned)time(NULL));

    // Default shows
    strcpy(shows[0].title, "Dune: Part Two");  shows[0].price = 12.50f;
    strcpy(shows[1].title, "Oppenheimer");     shows[1].price = 10.00f;
    strcpy(shows[2].title, "The Batman");      shows[2].price = 11.00f;
    for (int i = 0; i < MAX_SHOWS; i++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                shows[i].seats[r][c] = 0;

    printf("=================================\n");
    printf("   WELCOME TO MOVIE TICKET SYSTEM\n");
    printf("=================================\n");
    show_main_menu();
    return 0;
}

// ────────────────────────────────────────────────
// Show main menu
// ────────────────────────────────────────────────
void show_main_menu(void) {
    state = 0;
    printf("\n--- MAIN MENU ---\n");
    printf("1. View Shows & Seats\n");
    printf("2. Book Tickets\n");
    printf("3. View Booking by ID\n");
    printf("4. Occupancy Report\n");
    printf("5. Exit\n");
    printf("Enter choice: ");
    fflush(stdout);
}

// ────────────────────────────────────────────────
// Display seat map
// ────────────────────────────────────────────────
void display_seats(int idx) {
    printf("\n--- %s ($%.2f) ---\n", shows[idx].title, shows[idx].price);
    printf("     ");
    for (int j = 1; j <= COLS; j++) printf("%2d ", j);
    printf("\n");
    for (int r = 0; r < ROWS; r++) {
        printf("  %c |", 'A' + r);
        for (int c = 0; c < COLS; c++)
            printf(" %s ", shows[idx].seats[r][c] ? "X" : ".");
        printf("\n");
    }
    printf("  . = available   X = booked\n");
    fflush(stdout);
}

// ────────────────────────────────────────────────
// Occupancy report
// ────────────────────────────────────────────────
void occupancy_report(void) {
    printf("\n--- OCCUPANCY REPORT ---\n");
    for (int i = 0; i < MAX_SHOWS; i++) {
        int booked = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (shows[i].seats[r][c]) booked++;
        printf("%-20s  Booked: %2d/%d  (%.0f%%)\n",
               shows[i].title, booked, ROWS*COLS,
               booked * 100.0 / (ROWS*COLS));
    }
    fflush(stdout);
}

// ────────────────────────────────────────────────
// THE KEY FUNCTION: called by JS on every input
// ────────────────────────────────────────────────
EMSCRIPTEN_KEEPALIVE
void handle_input(const char *input) {
    // Echo the input
    printf("> %s\n", input);

    if (state == 0) {
        // Main menu choice
        int choice = atoi(input);
        switch (choice) {
            case 1:
                state = 1;
                printf("\nEnter Show Number (1-%d): ", MAX_SHOWS);
                fflush(stdout);
                break;
            case 2:
                state = 2;
                printf("\nSelect Show to Book (1-%d): ", MAX_SHOWS);
                fflush(stdout);
                break;
            case 3:
                state = 6;
                printf("\nEnter Booking ID: ");
                fflush(stdout);
                break;
            case 4:
                occupancy_report();
                show_main_menu();
                break;
            case 5:
                printf("\nGoodbye! Thank you for using CineBook.\n");
                fflush(stdout);
                break;
            default:
                printf("Invalid choice. Try again.\n");
                show_main_menu();
        }
    }

    else if (state == 1) {
        // View seats - show selection
        int s = atoi(input) - 1;
        if (s >= 0 && s < MAX_SHOWS) {
            display_seats(s);
        } else {
            printf("Invalid show number.\n");
        }
        show_main_menu();
    }

    else if (state == 2) {
        // Book - show selection
        int s = atoi(input) - 1;
        if (s >= 0 && s < MAX_SHOWS) {
            selected_show = s;
            display_seats(s);
            state = 3;
            printf("\nHow many seats to book? ");
            fflush(stdout);
        } else {
            printf("Invalid show number.\n");
            show_main_menu();
        }
    }

    else if (state == 3) {
        // Number of seats
        int n = atoi(input);
        if (n < 1 || n > ROWS*COLS) {
            printf("Invalid number. Enter 1-%d.\n", ROWS*COLS);
            printf("How many seats to book? ");
            fflush(stdout);
        } else {
            seats_to_book = n;
            seats_entered = 0;
            booked_seats_str[0] = '\0';
            state = 4;
            printf("\nEnter customer name: ");
            fflush(stdout);
        }
    }

    else if (state == 4) {
        // Customer name
        if (strlen(input) == 0) {
            printf("Name cannot be empty. Enter customer name: ");
            fflush(stdout);
        } else {
            strncpy(customer_name, input, 49);
            customer_name[49] = '\0';
            state = 5;
            printf("\nEnter seat %d of %d (format: A3 or B7): ",
                   seats_entered + 1, seats_to_book);
            fflush(stdout);
        }
    }

    else if (state == 5) {
        // Seat entry (e.g. "A3", "B10")
        if (strlen(input) < 2) {
            printf("Invalid format. Use letter+number e.g. A3\n");
            printf("Enter seat %d of %d: ", seats_entered + 1, seats_to_book);
            fflush(stdout);
            return;
        }

        char rowChar = input[0];
        int  col     = atoi(input + 1) - 1;
        int  row     = rowChar - 'A';

        // Case-insensitive
        if (rowChar >= 'a' && rowChar <= 'z') row = rowChar - 'a';

        if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
            printf("Invalid seat. Rows A-%c, Cols 1-%d.\n", 'A'+ROWS-1, COLS);
            printf("Enter seat %d of %d: ", seats_entered + 1, seats_to_book);
            fflush(stdout);
            return;
        }

        if (shows[selected_show].seats[row][col] == 1) {
            printf("Seat %c%d is already booked! Choose another: ", 'A'+row, col+1);
            fflush(stdout);
            return;
        }

        // Check duplicate in current selection
        for (int i = 0; i < seats_entered; i++) {
            if (pending_seats[i][0] == row && pending_seats[i][1] == col) {
                printf("You already selected %c%d! Choose another: ", 'A'+row, col+1);
                fflush(stdout);
                return;
            }
        }

        pending_seats[seats_entered][0] = row;
        pending_seats[seats_entered][1] = col;

        char tag[8];
        snprintf(tag, sizeof(tag), "%c%d", 'A'+row, col+1);
        if (seats_entered > 0) strcat(booked_seats_str, ", ");
        strcat(booked_seats_str, tag);

        seats_entered++;
        printf("Seat %s selected.\n", tag);

        if (seats_entered < seats_to_book) {
            printf("Enter seat %d of %d: ", seats_entered + 1, seats_to_book);
            fflush(stdout);
        } else {
            // Finalize booking
            float total = seats_to_book * shows[selected_show].price;
            int   bid   = nextBookingID++;

            for (int i = 0; i < seats_entered; i++)
                shows[selected_show].seats[pending_seats[i][0]][pending_seats[i][1]] = 1;

            Booking *b = &bookings[bookingCount++];
            b->bookingID   = bid;
            b->showIndex   = selected_show;
            b->numSeats    = seats_to_book;
            b->totalAmount = total;
            strncpy(b->customerName,  customer_name,    49);
            strncpy(b->seatPositions, booked_seats_str, 199);

            printf("\n=============================\n");
            printf("  BOOKING CONFIRMED!\n");
            printf("=============================\n");
            printf("  Booking ID : %d\n", bid);
            printf("  Name       : %s\n", b->customerName);
            printf("  Show       : %s\n", shows[selected_show].title);
            printf("  Seats      : %s\n", b->seatPositions);
            printf("  Total Paid : $%.2f\n", total);
            printf("=============================\n");
            fflush(stdout);

            show_main_menu();
        }
    }

    else if (state == 6) {
        // View booking by ID
        int id = atoi(input);
        int found = 0;
        for (int i = 0; i < bookingCount; i++) {
            if (bookings[i].bookingID == id) {
                printf("\n--- BOOKING #%d ---\n", bookings[i].bookingID);
                printf("Name   : %s\n", bookings[i].customerName);
                printf("Show   : %s\n", shows[bookings[i].showIndex].title);
                printf("Seats  : %s\n", bookings[i].seatPositions);
                printf("Amount : $%.2f\n", bookings[i].totalAmount);
                fflush(stdout);
                found = 1;
                break;
            }
        }
        if (!found) printf("Booking ID %d not found.\n", id);
        show_main_menu();
    }
}
