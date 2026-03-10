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

// ——————————————————————————————————————————————————
// Structures
// ——————————————————————————————————————————————————
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

// ——————————————————————————————————————————————————
// Globals
// ——————————————————————————————————————————————————
Show shows[MAX_SHOWS];
static int show_menu = 1;
static int sub_state = 0;           // 0=Main, 1=View, 2=Book(Show), 3=Book(Count), 4=Book(Name), 5=Book(Seats)
static int selected_show = -1;
static int seats_to_book = 0;
static int seats_entered = 0;
char current_name[50];
char current_seats_str[100];

// ——————————————————————————————————————————————————
// Prototypes
// ——————————————————————————————————————————————————
void displaySeats(int showIdx);
void viewBooking();
void showOccupancyReport();
void saveToFile();
void loadFromFile();
void clear_input_buffer();
int parseSeat(char *input, int *r, int *c);
void finalizeBooking();

// ——————————————————————————————————————————————————
// Helper Functions
// ——————————————————————————————————————————————————
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int parseSeat(char *input, int *r, int *c) {
    if (strlen(input) < 2) return 0;
    char rowPart = input[0];
    if (rowPart >= 'a' && rowPart <= 'z') *r = rowPart - 'a';
    else if (rowPart >= 'A' && rowPart <= 'Z') *r = rowPart - 'A';
    else return 0;
    
    *c = atoi(&input[1]) - 1;
    return (*r >= 0 && *r < ROWS && *c >= 0 && *c < COLS);
}

// ——————————————————————————————————————————————————
// Main Loop
// ——————————————————————————————————————————————————

    void main_loop() {
    // State 0: Display the Main Menu
    if (show_menu) {
        printf("\n--- MOVIE TICKET SYSTEM ---\n");
        printf("1. View Shows & Seats\n2. Book Tickets\n3. View Booking by ID\n4. Occupancy Report\n5. Exit\n");
        printf("Enter choice: ");
        fflush(stdout);
        show_menu = 0;
        sub_state = 0;
    }

    int num;
    // Check if there is integer input available
    if (scanf("%d", &num) == 1) {
        clear_input_buffer();

        if (sub_state == 0) {
            if (num == 1) { 
                printf("Enter Show Number (1-3): "); 
                sub_state = 1; 
            }
            else if (num == 2) { 
                printf("Select Show (1-3): "); 
                sub_state = 2; 
            }
            else if (num == 3) { viewBooking(); show_menu = 1; }
            else if (num == 4) { showOccupancyReport(); show_menu = 1; }
            else if (num == 5) { saveToFile(); emscripten_cancel_main_loop(); }
            fflush(stdout);
        }
        else if (sub_state == 1) {
            // View Only Mode
            if (num >= 1 && num <= MAX_SHOWS) displaySeats(num - 1);
            show_menu = 1; // Return to menu
            sub_state = 0;
        }
        else if (sub_state == 2) {
            // Booking Mode: Show selection
            if (num >= 1 && num <= MAX_SHOWS) {
                selected_show = num - 1;
                displaySeats(selected_show);
                // CRITICAL: Immediately ask for the next piece of info
                printf("How many seats do you want to book? ");
                fflush(stdout);
                sub_state = 3; // Move to the 'Number of Seats' state
            } else {
                printf("Invalid show. Select Show (1-3): ");
                fflush(stdout);
            }
        }
        else if (sub_state == 3) {
            // Number of Seats state
            seats_to_book = num;
            if (seats_to_book > 0 && seats_to_book <= (ROWS * COLS)) {
                printf("Enter customer name (no spaces): ");
                fflush(stdout);
                sub_state = 4; // Move to 'Name' state
            } else {
                printf("Invalid number. How many seats? ");
                fflush(stdout);
            }
        }
    }
    // Note: sub_state 4 (Name) and 5 (Seat selection) 
    // should follow the string input logic provided in the previous message.
}
    else if (sub_state == 4) {
        if (scanf("%s", current_name) == 1) {
            clear_input_buffer();
            seats_entered = 0;
            current_seats_str[0] = '\0';
            printf("Enter seat %d/%d (e.g., A1): ", seats_entered + 1, seats_to_book);
            sub_state = 5;
            fflush(stdout);
        }
    }
    else if (sub_state == 5) {
        char seatInput[10];
        if (scanf("%s", seatInput) == 1) {
            clear_input_buffer();
            int r, c;
            if (parseSeat(seatInput, &r, &c)) {
                if (shows[selected_show].seats[r][c] == 0) {
                    shows[selected_show].seats[r][c] = 1;
                    strcat(current_seats_str, seatInput);
                    strcat(current_seats_str, " ");
                    seats_entered++;
                    
                    if (seats_entered < seats_to_book) {
                        printf("Enter seat %d/%d: ", seats_entered + 1, seats_to_book);
                    } else {
                        finalizeBooking();
                        sub_state = 0;
                        show_menu = 1;
                    }
                } else { printf("Seat occupied! Try again: "); }
            } else { printf("Invalid format (A1-E10). Try again: "); }
            fflush(stdout);
        }
    }
}

int main() {
    srand(time(NULL));
    loadFromFile();
    printf("System Ready.\n");
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}

// ——————————————————————————————————————————————————
// Logic Implementation
// ——————————————————————————————————————————————————

void finalizeBooking() {
    Booking b;
    b.bookingID = rand() % 9000 + 1000;
    strcpy(b.customerName, current_name);
    b.showIndex = selected_show;
    b.numSeats = seats_to_book;
    strcpy(b.seatPositions, current_seats_str);
    b.totalAmount = seats_to_book * shows[selected_show].price;

    FILE *fp = fopen(BOOKINGS_FILE, "ab");
    if (fp) {
        fwrite(&b, sizeof(Booking), 1, fp);
        fclose(fp);
    }
    saveToFile();
    printf("\nSuccess! Booking ID: %d | Total: $%.2f\n", b.bookingID, b.totalAmount);
}

void displaySeats(int showIdx) {
    printf("\n--- %s ---\n    ", shows[showIdx].title);
    for (int j = 1; j <= COLS; j++) printf("%2d ", j);
    printf("\n");
    for (int i = 0; i < ROWS; i++) {
        printf("%c | ", 'A' + i);
        for (int j = 0; j < COLS; j++) printf("%s  ", shows[showIdx].seats[i][j] ? "X" : ".");
        printf("\n");
    }
}

void viewBooking() {
    int id;
    printf("Enter Booking ID: ");
    fflush(stdout);
    if (scanf("%d", &id) != 1) { clear_input_buffer(); return; }
    clear_input_buffer();

    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp) { printf("No bookings found.\n"); return; }
    Booking b;
    int found = 0;
    while (fread(&b, sizeof(Booking), 1, fp)) {
        if (b.bookingID == id) {
            printf("\n--- BOOKING %d ---\nName: %s\nMovie: %s\nSeats: %s\nTotal: $%.2f\n", 
                b.bookingID, b.customerName, shows[b.showIndex].title, b.seatPositions, b.totalAmount);
            found = 1; break;
        }
    }
    fclose(fp);
    if (!found) printf("ID %d not found.\n", id);
}

void showOccupancyReport() {
    printf("\n--- REPORT ---\n");
    for (int i = 0; i < MAX_SHOWS; i++) {
        int booked = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) if (shows[i].seats[r][c]) booked++;
        printf("%-15s: %d/%d booked (%d%%)\n", shows[i].title, booked, ROWS*COLS, (booked*100)/(ROWS*COLS));
    }
}

void saveToFile() {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (fp) { fwrite(shows, sizeof(Show), MAX_SHOWS, fp); fclose(fp); }
}

void loadFromFile() {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (fp) { fread(shows, sizeof(Show), MAX_SHOWS, fp); fclose(fp); } 
    else {
        strcpy(shows[0].title, "Dune: Part Two");   shows[0].price = 12.0;
        strcpy(shows[1].title, "Oppenheimer");      shows[1].price = 10.0;
        strcpy(shows[2].title, "The Batman");       shows[2].price = 11.0;
        for (int i = 0; i < MAX_SHOWS; i++)
            for (int r = 0; r < ROWS; r++)
                for (int c = 0; c < COLS; c++) shows[i].seats[r][c] = 0;
    }
}