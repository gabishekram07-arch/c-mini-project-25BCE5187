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
    char seatPositions[200];
    float totalAmount;
} Booking;

Show shows[MAX_SHOWS];

static int show_menu = 1;
static int sub_state = 0;

static int selected_show = -1;
static int seats_to_book = 0;
static int seats_entered = 0;

char current_name[50];
char current_seats_str[200];

void displaySeats(int showIdx);
void showOccupancyReport();
void saveToFile();
void loadFromFile();
int parseSeat(char *input, int *r, int *c);
void finalizeBooking();
void searchBookingByID(int id);

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
    if (*r >= 0 && *r < ROWS && *c >= 0 && *c < COLS) return 1;
    return 0;
}

void main_loop() {
    static char input[100];

    // INITIAL MENU PRINTING
    if (show_menu) {
        printf("\n===== MOVIE TICKET SYSTEM =====\n");
        printf("1. View Shows & Seats\n");
        printf("2. Book Tickets\n");
        printf("3. View Booking by ID\n");
        printf("4. Occupancy Report\n");
        printf("5. Exit\n");
        printf("Enter choice: "); // Prompt appears before input
        fflush(stdout);
        show_menu = 0;
        return;
    }

    // Wait for input from the HTML text box
    if (!fgets(input, sizeof(input), stdin))
        return;

    trimnewline(input);

    if (sub_state == 0) {
        int num = atoi(input);
        if (num == 1) {
            printf("\nEnter Show Number (1-3) to view: ");
            fflush(stdout);
            sub_state = 1;
        } else if (num == 2) {
            printf("\nSelect Show (1-3) to book: ");
            fflush(stdout);
            sub_state = 2;
        } else if (num == 3) {
            printf("\nEnter Booking ID to search: ");
            fflush(stdout);
            sub_state = 6;
        } else if (num == 4) {
            showOccupancyReport();
            show_menu = 1; // Resets to main menu
        } else if (num == 5) {
            printf("\nExiting...\n");
            saveToFile();
            emscripten_cancel_main_loop();
        } else {
            printf("\nInvalid choice. Enter choice: ");
            fflush(stdout);
        }
    } 
    else if (sub_state == 1) {
        int s = atoi(input) - 1;
        if (s >= 0 && s < MAX_SHOWS) displaySeats(s);
        else printf("\nInvalid show number.");
        show_menu = 1;
        sub_state = 0;
    } 
    else if (sub_state == 2) {
        int s = atoi(input) - 1;
        if (s >= 0 && s < MAX_SHOWS) {
            selected_show = s;
            displaySeats(s);
            printf("\nSelected Movie: %s", shows[s].title);
            printf("\nHow many seats: ");
            fflush(stdout);
            sub_state = 3;
        } else {
            printf("\nInvalid show. Select (1-3): ");
            fflush(stdout);
        }
    } 
    else if (sub_state == 3) {
        seats_to_book = atoi(input);
        if (seats_to_book <= 0) {
            printf("\nInvalid number. How many seats: ");
            fflush(stdout);
        } else {
            printf("\nEnter customer name: ");
            fflush(stdout);
            sub_state = 4;
        }
    } 
    else if (sub_state == 4) {
        strcpy(current_name, input);
        seats_entered = 0;
        current_seats_str[0] = '\0';
        printf("\nEnter seat 1/%d (e.g., A1): ", seats_to_book);
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
                    printf("\nEnter seat %d/%d: ", seats_entered + 1, seats_to_book);
                    fflush(stdout);
                } else {
                    finalizeBooking();
                    sub_state = 0;
                    show_menu = 1;
                }
            } else {
                printf("\nSeat already booked. Try again: ");
                fflush(stdout);
            }
        } else {
            printf("\nInvalid seat format (A1-E10). Try again: ");
            fflush(stdout);
        }
    } 
    else if (sub_state == 6) {
        int id = atoi(input);
        searchBookingByID(id);
        sub_state = 0;
        show_menu = 1;
    }
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for immediate output
    srand(time(NULL));
    loadFromFile();
    printf("System Ready\n");
    fflush(stdout);
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}

// ... Rest of your functions (searchBookingByID, finalizeBooking, etc.) remain the same ...

void searchBookingByID(int id) {
    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp) { printf("\nNo bookings found\n"); return; }
    Booking b;
    int found = 0;
    while (fread(&b, sizeof(Booking), 1, fp)) {
        if (b.bookingID == id) {
            printf("\nBooking ID: %d\nCustomer: %s\nMovie: %s\nSeats: %s\nTotal: $%.2f\n",
                   b.bookingID, b.customerName, shows[b.showIndex].title, b.seatPositions, b.totalAmount);
            found = 1;
            break;
        }
    }
    fclose(fp);
    if (!found) printf("\nBooking not found\n");
}

void finalizeBooking() {
    Booking b;
    b.bookingID = rand() % 9000 + 1000;
    strcpy(b.customerName, current_name);
    b.showIndex = selected_show;
    b.numSeats = seats_to_book;
    strcpy(b.seatPositions, current_seats_str);
    b.totalAmount = seats_to_book * shows[selected_show].price;
    FILE *fp = fopen(BOOKINGS_FILE, "ab");
    if (fp) { fwrite(&b, sizeof(Booking), 1, fp); fclose(fp); }
    saveToFile();
    printf("\nBooking Successful!\nBooking ID: %d\nTotal Amount: $%.2f\n", b.bookingID, b.totalAmount);
}

void displaySeats(int showIdx) {
    printf("\n--- %s ---\n    ", shows[showIdx].title);
    for (int i = 1; i <= COLS; i++) printf("%2d ", i);
    printf("\n");
    for (int r = 0; r < ROWS; r++) {
        printf("%c | ", 'A' + r);
        for (int c = 0; c < COLS; c++) {
            if (shows[showIdx].seats[r][c]) printf("X  ");
            else printf(".  ");
        }
        printf("\n");
    }
}

void showOccupancyReport() {
    printf("\n=== OCCUPANCY REPORT ===\n");
    for (int i = 0; i < MAX_SHOWS; i++) {
        int booked = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (shows[i].seats[r][c]) booked++;
        printf("%s : %d/%d seats booked\n", shows[i].title, booked, ROWS * COLS);
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
        strcpy(shows[0].title, "Dune Part Two"); shows[0].price = 12;
        strcpy(shows[1].title, "Oppenheimer"); shows[1].price = 10;
        strcpy(shows[2].title, "The Batman"); shows[2].price = 11;
        for (int i = 0; i < MAX_SHOWS; i++)
            for (int r = 0; r < ROWS; r++)
                for (int c = 0; c < COLS; c++) shows[i].seats[r][c] = 0;
    }
}