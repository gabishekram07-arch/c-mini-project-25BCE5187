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

typedef struct {
    char title[50];
    int seats[ROWS][COLS];
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

Show shows[MAX_SHOWS];
static int show_menu = 1;
static int sub_state = 0; 
static int selected_show = -1;
static int seats_to_book = 0;
static int seats_entered = 0;

char current_name[50];
char current_seats_str[100];

// Prototypes
void displaySeats(int showIdx);
void viewBooking();
void showOccupancyReport();
void saveToFile();
void loadFromFile();
int parseSeat(char *input, int *r, int *c);
void finalizeBooking();

void trimnewline(char *str) {
    str[strcspn(str, "\n")] = 0;
}

int parseSeat(char *input, int *r, int *c) {
    if (strlen(input) < 2) return 0;
    char row = input[0];
    if (row >= 'A' && row <= 'Z') *r = row - 'A';
    else if (row >= 'a' && row <= 'z') *r = row - 'a';
    else return 0;
    *c = atoi(&input[1]) - 1;
    return (*r >= 0 && *r < ROWS && *c >= 0 && *c < COLS);
}

void main_loop() {
    if (show_menu) {
        printf("\n===== MOVIE TICKET SYSTEM =====\n");
        printf("1. View Shows & Seats\n2. Book Tickets\n3. View Booking by ID\n4. Occupancy Report\n5. Exit\n");
        printf("\nEnter choice: ");
        fflush(stdout);
        show_menu = 0;
        sub_state = 0;
        return; // Exit loop to wait for input
    }

    char input[100];
    if (!fgets(input, sizeof(input), stdin)) return;
    trimnewline(input);

    if (sub_state == 0) {
        int num = atoi(input);
        if (num == 1) { printf("Enter Show Number (1-3): "); fflush(stdout); sub_state = 1; }
        else if (num == 2) { printf("Select Show (1-3): "); fflush(stdout); sub_state = 2; }
        else if (num == 3) { viewBooking(); show_menu = 1; }
        else if (num == 4) { showOccupancyReport(); show_menu = 1; }
        else if (num == 5) { saveToFile(); emscripten_cancel_main_loop(); }
    } 
    else if (sub_state == 1) {
        int s = atoi(input) - 1;
        if (s >= 0 && s < MAX_SHOWS) displaySeats(s);
        else printf("Invalid show number\n");
        show_menu = 1;
    } 
    else if (sub_state == 2) {
        int s = atoi(input) - 1;
        if (s >= 0 && s < MAX_SHOWS) {
            selected_show = s;
            displaySeats(s);
            printf("How many seats: ");
            fflush(stdout);
            sub_state = 3;
        } else {
            printf("Invalid show. Select Show (1-3): ");
            fflush(stdout);
        }
    } 
    else if (sub_state == 3) {
        seats_to_book = atoi(input);
        if (seats_to_book <= 0) {
            printf("Invalid count. How many seats: ");
            fflush(stdout);
        } else {
            printf("Enter customer name: ");
            fflush(stdout);
            sub_state = 4;
        }
    } 
    else if (sub_state == 4) {
        strcpy(current_name, input);
        seats_entered = 0;
        current_seats_str[0] = '\0';
        printf("Enter seat %d/%d: ", seats_entered + 1, seats_to_book);
        fflush(stdout);
        sub_state = 5;
    } 
    else if (sub_state == 5) {
        int r, c;
        if (parseSeat(input, &r, &c)) {
            if (shows[selected_show].seats[r][c] == 0) {
                shows[selected_show].seats[r][c] = 1;
                strcat(current_seats_str, input);
                strcat(current_seats_str, " ");
                seats_entered++;
                if (seats_entered < seats_to_book) {
                    printf("Enter seat %d/%d: ", seats_entered + 1, seats_to_book);
                    fflush(stdout);
                } else {
                    finalizeBooking();
                    show_menu = 1;
                }
            } else {
                printf("Seat already booked. Try again: ");
                fflush(stdout);
            }
        } else {
            printf("Invalid format. Enter seat %d/%d (e.g. A1): ", seats_entered + 1, seats_to_book);
            fflush(stdout);
        }
    }
}

int main() {
    srand(time(NULL));
    loadFromFile();
    printf("System Ready\n");
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}

// ... Keep your existing finalizeBooking, displaySeats, viewBooking, 
// showOccupancyReport, saveToFile, and loadFromFile functions exactly as they were ...

void finalizeBooking()
{
    static int next_booking_id = 1000;  // Persistent ID counter
    Booking b;
    b.bookingID = next_booking_id++;
    strcpy(b.customerName, current_name);
    b.showIndex = selected_show;
    b.numSeats = seats_to_book;
    strcpy(b.seatPositions, current_seats_str);
    b.totalAmount = seats_to_book * shows[selected_show].price;

    FILE *fp = fopen(BOOKINGS_FILE, "ab");
    if(fp) {
        fwrite(&b, sizeof(Booking), 1, fp);
        fclose(fp);
    }

    saveToFile();

    printf("\n🎉 Booking Successful!\n");
    printf("Booking ID: %d\n", b.bookingID);
    printf("Seats: %s\n", b.seatPositions);
    printf("Total Amount: $%.2f\n", b.totalAmount);
    printf("Thank you, %s!\n", b.customerName);
}

void displaySeats(int showIdx)
{
    printf("\n--- %s (Price: $%.2f) ---\n", shows[showIdx].title, shows[showIdx].price);
    printf("    ");
    for(int i = 1; i <= COLS; i++) printf("%2d ", i);
    printf("\n");

    for(int r = 0; r < ROWS; r++) {
        printf("%c | ", 'A' + r);
        for(int c = 0; c < COLS; c++) {
            if(shows[showIdx].seats[r][c])
                printf(" X ");
            else
                printf(" . ");
        }
        printf("\n");
    }
    printf("\n");
}

void viewBooking()
{
    char input[100];
    printf("Enter Booking ID: ");
    fflush(stdout);
    if(!fgets(input, sizeof(input), stdin)) return;
    trimnewline(input);
    int id = atoi(input);

    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if(!fp) {
        printf("No bookings found.\n");
        return;
    }

    Booking b;
    int found = 0;
    while(fread(&b, sizeof(Booking), 1, fp)) {
        if(b.bookingID == id) {
            printf("\n--- Booking Details ---\n");
            printf("ID: %d\n", b.bookingID);
            printf("Customer: %s\n", b.customerName);
            printf("Movie: %s\n", shows[b.showIndex].title);
            printf("Seats (%d): %s\n", b.numSeats, b.seatPositions);
            printf("Total: $%.2f\n", b.totalAmount);
            found = 1;
            break;
        }
    }
    fclose(fp);
    if(!found) printf("Booking ID %d not found.\n", id);
}

void showOccupancyReport()
{
    printf("\n=== OCCUPANCY REPORT ===\n");
    int total_booked = 0;
    for(int i = 0; i < MAX_SHOWS; i++) {
        int booked = 0;
        for(int r = 0; r < ROWS; r++)
            for(int c = 0; c < COLS; c++)
                if(shows[i].seats[r][c]) booked++;
        total_booked += booked;
        float pct = (float)booked / (ROWS * COLS) * 100;
        printf("%s: %d/%d seats (%.1f%%)\n", shows[i].title, booked, ROWS*COLS, pct);
    }
    printf("Overall: %d/%d seats booked\n", total_booked, MAX_SHOWS * ROWS * COLS);
}

void saveToFile()
{
    FILE *fp = fopen(DATA_FILE, "wb");
    if(fp) {
        fwrite(shows, sizeof(Show), MAX_SHOWS, fp);
        fclose(fp);
    }
}

void loadFromFile()
{
    FILE *fp = fopen(DATA_FILE, "rb");
    if(fp) {
        fread(shows, sizeof(Show), MAX_SHOWS, fp);
        fclose(fp);
    } else {
        // Default shows
        strcpy(shows[0].title, "Dune Part Two");
        shows[0].price = 12.0f;
        strcpy(shows[1].title, "Oppenheimer");
        shows[1].price = 10.0f;
        strcpy(shows[2].title, "The Batman");
        shows[2].price = 11.0f;

        for(int i = 0; i < MAX_SHOWS; i++)
            for(int r = 0; r < ROWS; r++)
                for(int c = 0; c < COLS; c++)
                    shows[i].seats[r][c] = 0;
    }
}
