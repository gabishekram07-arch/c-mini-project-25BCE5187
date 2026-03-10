#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

#define ROWS 5
#define COLS 10
#define MAX_SHOWS 3
#define DATA_FILE "theater_data.bin"

// Structure Definitions
typedef struct {
    char title[50];
    int seats[ROWS][COLS]; // 0 = Available, 1 = Booked
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

// Global Array of Shows
Show shows[MAX_SHOWS];

// Function Prototypes
void initializeShows();
void displaySeats(int showIdx);
void bookTicket();
void viewBooking();
void showOccupancyReport();
void saveToFile();
void loadFromFile();

// Helper function to handle input waiting
void wait_for_input() {
    clearerr(stdin);
    while(getchar() != '\n' && getchar() != EOF); 
    emscripten_sleep(500); // Wait half a second before allowing the loop to continue
}

int main() {
    loadFromFile(); 
    int choice;

    while (1) {
        // Essential: lets the browser process the UI/Input queue
        emscripten_sleep(100); 

        printf("\n--- MOVIE TICKET SYSTEM ---\n");
        printf("1. View Shows & Seats\n");
        printf("2. Book Tickets\n");
        printf("3. View Booking by ID\n");
        printf("4. Occupancy Report\n");
        printf("5. Exit\n");
        printf("Enter choice: ");
        fflush(stdout); 
        
        // If scanf fails, it means no text has been sent from the HTML box yet
        if (scanf("%d", &choice) != 1) {
            wait_for_input(); 
            continue; // Restarts the loop silently
        }

        switch (choice) {
            case 1:
                for(int i=0; i<MAX_SHOWS; i++) {
                    printf("\nShow [%d]: %s", i+1, shows[i].title);
                }
                printf("\nEnter Show Number (1-%d): ", MAX_SHOWS);
                fflush(stdout);
                int sIdx; 
                while(scanf("%d", &sIdx) != 1) { wait_for_input(); }
                if(sIdx >= 1 && sIdx <= MAX_SHOWS) displaySeats(sIdx-1);
                else printf("Invalid Show.\n");
                break;
            case 2: bookTicket(); break;
            case 3: viewBooking(); break;
            case 4: showOccupancyReport(); break;
            case 5: saveToFile(); exit(0);
            default: printf("Invalid choice!\n");
        }
    }
    return 0;
}

void displaySeats(int showIdx) {
    printf("\n--- Seat Layout for %s ---\n", shows[showIdx].title);
    printf("    ");
    for(int j=1; j<=COLS; j++) printf("%d  ", j);
    printf("\n");

    for(int i=0; i<ROWS; i++) {
        printf("%c |", 'A' + i);
        for(int j=0; j<COLS; j++) {
            if(shows[showIdx].seats[i][j] == 0) printf(".  "); 
            else printf("X  "); 
        }
        printf("\n");
    }
    printf("Legend: [ . ] Available  [ X ] Booked\n");
    fflush(stdout);
}

void bookTicket() {
    int sIdx, n;
    printf("\nSelect Show (1-%d): ", MAX_SHOWS);
    fflush(stdout);
    while(scanf("%d", &sIdx) != 1) { wait_for_input(); }
    sIdx--; 

    if(sIdx < 0 || sIdx >= MAX_SHOWS) {
        printf("Invalid show selection.\n");
        return;
    }

    displaySeats(sIdx);
    printf("Enter number of seats: ");
    fflush(stdout);
    while(scanf("%d", &n) != 1) { wait_for_input(); }

    Booking b;
    b.bookingID = rand() % 9000 + 1000;
    printf("Enter Customer Name (no spaces): ");
    fflush(stdout);
    while(scanf("%s", b.customerName) != 1) { wait_for_input(); }
    
    b.showIndex = sIdx;
    b.numSeats = n;
    b.totalAmount = n * shows[sIdx].price;
    strcpy(b.seatPositions, "");

    for(int k=0; k<n; k++) {
        char r; int c;
        printf("Enter seat %d (e.g., A 5): ", k+1);
        fflush(stdout);
        while(scanf(" %c %d", &r, &c) != 2) { wait_for_input(); }
        
        int row = r - 'A';
        int col = c - 1;

        if(row < 0 || row >= ROWS || col < 0 || col >= COLS) {
            printf("Invalid seat!\n");
            k--; continue;
        }
        if(shows[sIdx].seats[row][col] == 1) {
            printf("Seat already booked!\n");
            k--; continue;
        }

        shows[sIdx].seats[row][col] = 1;
        char temp[10];
        sprintf(temp, "%c%d ", r, c);
        strcat(b.seatPositions, temp);
    }

    FILE *fp = fopen("bookings.bin", "ab");
    if(fp) {
        fwrite(&b, sizeof(Booking), 1, fp);
        fclose(fp);
    }

    printf("\n--- BOOKING SUCCESSFUL ---");
    printf("\nID: %d | Seats: %s | Total: $%.2f\n", b.bookingID, b.seatPositions, b.totalAmount);
    saveToFile(); 
}

void viewBooking() {
    int id, found = 0;
    printf("Enter Booking ID: ");
    fflush(stdout);
    while(scanf("%d", &id) != 1) { wait_for_input(); }

    FILE *fp = fopen("bookings.bin", "rb");
    if(!fp) { printf("No bookings found.\n"); return; }

    Booking b;
    while(fread(&b, sizeof(Booking), 1, fp)) {
        if(b.bookingID == id) {
            printf("\n--- RECEIPT ---\nID: %d\nName: %s\nSeats: %s\n", 
                    b.bookingID, b.customerName, b.seatPositions);
            found = 1;
            break;
        }
    }
    if(!found) printf("ID not found.\n");
    fclose(fp);
}

void showOccupancyReport() {
    printf("\n--- OCCUPANCY REPORT ---\n");
    for(int i=0; i<MAX_SHOWS; i++) {
        int booked = 0;
        for(int r=0; r<ROWS; r++) {
            for(int c=0; c<COLS; c++) {
                if(shows[i].seats[r][c] == 1) booked++;
            }
        }
        printf("Movie: %s | Booked: %d/%d\n", shows[i].title, booked, ROWS*COLS);
    }
}

void saveToFile() {
    FILE *fp = fopen(DATA_FILE, "wb");
    if(fp) {
        fwrite(shows, sizeof(Show), MAX_SHOWS, fp);
        fclose(fp);
    }
}

void loadFromFile() {
    FILE *fp = fopen(DATA_FILE, "rb");
    if(!fp) {
        strcpy(shows[0].title, "Dune: Part Two"); shows[0].price = 12.50;
        strcpy(shows[1].title, "Oppenheimer"); shows[1].price = 10.00;
        strcpy(shows[2].title, "Batman"); shows[2].price = 11.00;
        for(int i=0; i<MAX_SHOWS; i++)
            for(int r=0; r<ROWS; r++)
                for(int c=0; c<COLS; c++) shows[i].seats[r][c] = 0;
    } else {
        fread(shows, sizeof(Show), MAX_SHOWS, fp);
        fclose(fp);
    }
}