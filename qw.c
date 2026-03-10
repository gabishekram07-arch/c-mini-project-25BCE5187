#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <emscripten/emscripten.h>

#define ROWS 5
#define COLS 10
#define MAX_SHOWS 3
#define DATA_FILE "theater_data.bin"
#define BOOKINGS_FILE "bookings.bin"

// ────────────────────────────────────────────────
// Structures
// ────────────────────────────────────────────────
typedef struct {
    char title[50];
    int seats[ROWS][COLS];      // 0 = available, 1 = booked
    float price;
} Show;

typedef struct {
    int bookingID;
    char customerName[50];
    int showIndex;
    int numSeats;
    char seatPositions[100];
    float totalAmount;
} Booking;

// ────────────────────────────────────────────────
// Globals
// ────────────────────────────────────────────────
Show shows[MAX_SHOWS];

static int show_menu = 1;
static int current_choice = 0;
static int sub_state = 0;           // 0 = main menu, 1 = select show for view, 2 = select show for booking, etc.
static int selected_show = -1;
static int seats_to_book = 0;
static int seats_entered = 0;

// ────────────────────────────────────────────────
// Function prototypes
// ────────────────────────────────────────────────
void initializeShows();
void displaySeats(int showIdx);
void bookTicketStart();
void continueBooking();
void viewBooking();
void showOccupancyReport();
void saveToFile();
void loadFromFile();
void clear_input_buffer(void);

// ────────────────────────────────────────────────
// Main loop (browser friendly)
// ────────────────────────────────────────────────
void main_loop() {
    if (show_menu) {
        printf("\n--- MOVIE TICKET SYSTEM ---\n");
        printf("1. View Shows & Seats\n");
        printf("2. Book Tickets\n");
        printf("3. View Booking by ID\n");
        printf("4. Occupancy Report\n");
        printf("5. Exit\n");
        printf("Enter choice: ");
        fflush(stdout);
        show_menu = 0;
        sub_state = 0;
    }

    int num;
    if (scanf("%d", &num) == 1) {
        clear_input_buffer();

        if (sub_state == 0) {
            // Main menu choice
            current_choice = num;
            show_menu = 1;

            switch (current_choice) {
                case 1:
                    printf("\nEnter Show Number (1-3): ");
                    fflush(stdout);
                    sub_state = 1;
                    show_menu = 0;
                    break;

                case 2:
                    printf("\nSelect Show (1-3): ");
                    fflush(stdout);
                    sub_state = 2;
                    show_menu = 0;
                    break;

                case 3:
                    viewBooking();
                    break;

                case 4:
                    showOccupancyReport();
                    break;

                case 5:
                    saveToFile();
                    printf("\nGoodbye! Data saved.\n");
                    emscripten_cancel_main_loop();
                    return;

                default:
                    printf("Invalid choice. Please try again.\n");
            }
        }
        else if (sub_state == 1) {
            // View seats → show selection
            int s = num - 1;
            if (s >= 0 && s < MAX_SHOWS) {
                displaySeats(s);
            } else {
                printf("Invalid show number.\n");
            }
            sub_state = 0;
        }
        else if (sub_state == 2) {
            // Booking → show selection
            int s = num - 1;
            if (s >= 0 && s < MAX_SHOWS) {
                selected_show = s;
                displaySeats(s);
                printf("\nHow many seats do you want to book? ");
                fflush(stdout);
                sub_state = 3;
            } else {
                printf("Invalid show number.\n");
                sub_state = 0;
            }
        }
        else if (sub_state == 3) {
            // Number of seats
            seats_to_book = num;
            if (seats_to_book < 1 || seats_to_book > ROWS*COLS) {
                printf("Invalid number of seats.\n");
                sub_state = 0;
            } else {
                seats_entered = 0;
                printf("Enter customer name (no spaces): ");
                fflush(stdout);
                sub_state = 4;
            }
        }
    }
    // If scanf didn't succeed → just loop quietly
}

// ────────────────────────────────────────────────
int main() {
    srand(time(NULL));
    loadFromFile();

    printf("Movie Ticket System loaded.\n");

    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}

// ────────────────────────────────────────────────
// Helper: clear leftover input
// ────────────────────────────────────────────────
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// ────────────────────────────────────────────────
// Display seat map
// ────────────────────────────────────────────────
void displaySeats(int showIdx) {
    printf("\n--- %s ---\n", shows[showIdx].title);
    printf("    ");
    for (int j = 1; j <= COLS; j++) printf("%2d ", j);
    printf("\n");

    for (int i = 0; i < ROWS; i++) {
        printf("%c | ", 'A' + i);
        for (int j = 0; j < COLS; j++) {
            printf("%s  ", shows[showIdx].seats[i][j] ? "X" : ".");
        }
        printf("\n");
    }
    printf(" . = available   X = booked\n\n");
    fflush(stdout);
}

// ────────────────────────────────────────────────
// Booking (continued in sub_state)
// ────────────────────────────────────────────────
void continueBooking() {
    // This is placeholder — full multi-seat input would need more states
    // For simplicity we show the concept only
    printf("[Booking logic partially implemented]\n");
    printf("Selected show: %s\n", shows[selected_show].title);
    printf("Seats requested: %d\n", seats_to_book);
    // ... add real seat picking logic here later
}

// ────────────────────────────────────────────────
// View single booking by ID
// ────────────────────────────────────────────────
void viewBooking() {
    int id;
    printf("Enter Booking ID: ");
    fflush(stdout);

    if (scanf("%d", &id) != 1) {
        clear_input_buffer();
        printf("Invalid input.\n");
        return;
    }
    clear_input_buffer();

    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp) {
        printf("No bookings found.\n");
        return;
    }

    Booking b;
    int found = 0;
    while (fread(&b, sizeof(Booking), 1, fp)) {
        if (b.bookingID == id) {
            printf("\n--- BOOKING FOUND ---\n");
            printf("ID       : %d\n", b.bookingID);
            printf("Name     : %s\n", b.customerName);
            printf("Seats    : %s\n", b.seatPositions);
            printf("Amount   : $%.2f\n", b.totalAmount);
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (!found) printf("Booking ID %d not found.\n", id);
}

// ────────────────────────────────────────────────
// Occupancy Report
// ────────────────────────────────────────────────
void showOccupancyReport() {
    printf("\n--- OCCUPANCY REPORT ---\n");
    for (int i = 0; i < MAX_SHOWS; i++) {
        int booked = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (shows[i].seats[r][c]) booked++;
        printf("%-20s  Booked: %2d / %d   (%.0f%%)\n",
               shows[i].title, booked, ROWS*COLS,
               (booked * 100.0) / (ROWS*COLS));
    }
    printf("\n");
}

// ────────────────────────────────────────────────
// File persistence
// ────────────────────────────────────────────────
void saveToFile() {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (fp) {
        fwrite(shows, sizeof(Show), MAX_SHOWS, fp);
        fclose(fp);
    }
}

void loadFromFile() {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (fp) {
        fread(shows, sizeof(Show), MAX_SHOWS, fp);
        fclose(fp);
    } else {
        // Default data
        strcpy(shows[0].title, "Dune: Part Two");   shows[0].price = 12.50f;
        strcpy(shows[1].title, "Oppenheimer");      shows[1].price = 10.00f;
        strcpy(shows[2].title, "The Batman");       shows[2].price = 11.00f;

        for (int i = 0; i < MAX_SHOWS; i++)
            for (int r = 0; r < ROWS; r++)
                for (int c = 0; c < COLS; c++)
                    shows[i].seats[r][c] = 0;
    }
}