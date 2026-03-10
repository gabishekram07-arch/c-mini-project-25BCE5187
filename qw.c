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

int main() {
    loadFromFile(); // Load existing data if available
    int choice;

    while (1) {
        emscripten_sleep(1);
        printf("\n--- MOVIE TICKET SYSTEM ---\n");
        printf("1. View Shows & Seats\n");
        printf("2. Book Tickets\n");
        printf("3. View Booking by ID\n");
        printf("4. Occupancy Report\n");
        printf("5. Exit\n");
        printf("Enter choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Restarting...\n");
            while(getchar() != '\n' && getchar() != EOF); // Clear buffer
            emscripten_sleep(100);
            continue;
        }

        switch (choice) {
            case 1:
                for(int i=0; i<MAX_SHOWS; i++) {
                    printf("\nShow [%d]: %s", i+1, shows[i].title);
                }
                printf("\nEnter Show Number (1-%d) to view seats: ", MAX_SHOWS);
                int sIdx; scanf("%d", &sIdx);
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

// 1. Function to display the 5x10 seat grid
void displaySeats(int showIdx) {
    printf("\n--- Seat Layout for %s ---\n", shows[showIdx].title);
    printf("    ");
    for(int j=1; j<=COLS; j++) printf("%d  ", j);
    printf("\n");

    for(int i=0; i<ROWS; i++) {
        printf("%c |", 'A' + i);
        for(int j=0; j<COLS; j++) {
            if(shows[showIdx].seats[i][j] == 0) printf(".  "); // Available
            else printf("X  "); // Booked
        }
        printf("\n");
    }
    printf("Legend: [ . ] Available  [ X ] Booked\n");
}

// 2. Function to handle booking logic
void bookTicket() {
    int sIdx, n;
    printf("\nSelect Show (1-%d): ", MAX_SHOWS);
    scanf("%d", &sIdx);
    sIdx--; // Convert to 0-index

    if(sIdx < 0 || sIdx >= MAX_SHOWS) {
        printf("Invalid show selection.\n");
        return;
    }

    displaySeats(sIdx);
    printf("Enter number of seats to book: ");
    scanf("%d", &n);

    Booking b;
    b.bookingID = rand() % 9000 + 1000; // Random 4-digit ID
    printf("Enter Customer Name: ");
    scanf("%s", b.customerName);
    
    b.showIndex = sIdx;
    b.numSeats = n;
    b.totalAmount = n * shows[sIdx].price;
    strcpy(b.seatPositions, "");

    for(int k=0; k<n; k++) {
        char r; int c;
        printf("Enter seat %d (e.g., A 5): ", k+1);
        scanf(" %c %d", &r, &c);
        
        int row = r - 'A';
        int col = c - 1;

        if(row < 0 || row >= ROWS || col < 0 || col >= COLS) {
            printf("Invalid seat! Try again.\n");
            k--; continue;
        }
        if(shows[sIdx].seats[row][col] == 1) {
            printf("Seat %c%d is already booked! Pick another.\n", r, c);
            k--; continue;
        }

        shows[sIdx].seats[row][col] = 1;
        char temp[10];
        sprintf(temp, "%c%d ", r, c);
        strcat(b.seatPositions, temp);
    }

    // Save booking to file
    FILE *fp = fopen("bookings.bin", "ab");
    fwrite(&b, sizeof(Booking), 1, fp);
    fclose(fp);

    printf("\n--- BOOKING SUCCESSFUL ---");
    printf("\nBooking ID: %d\nSeats: %s\nTotal: $%.2f\n", b.bookingID, b.seatPositions, b.totalAmount);
    saveToFile(); // Save the updated seat map
}

// 3. Function to view booking by ID
void viewBooking() {
    int id, found = 0;
    printf("Enter Booking ID: ");
    scanf("%d", &id);

    FILE *fp = fopen("bookings.bin", "rb");
    if(!fp) { printf("No bookings found.\n"); return; }

    Booking b;
    while(fread(&b, sizeof(Booking), 1, fp)) {
        if(b.bookingID == id) {
            printf("\n--- RECEIPT ---\n");
            printf("ID: %d\nName: %s\nMovie: %s\nSeats: %s\nTotal: $%.2f\n", 
                    b.bookingID, b.customerName, shows[b.showIndex].title, b.seatPositions, b.totalAmount);
            found = 1;
            break;
        }
    }
    if(!found) printf("Booking ID not found.\n");
    fclose(fp);
}

// 4. Occupancy Report Function
void showOccupancyReport() {
    printf("\n--- OCCUPANCY REPORT ---\n");
    for(int i=0; i<MAX_SHOWS; i++) {
        int booked = 0;
        for(int r=0; r<ROWS; r++) {
            for(int c=0; c<COLS; c++) {
                if(shows[i].seats[r][c] == 1) booked++;
            }
        }
        int total = ROWS * COLS;
        float perc = ((float)booked / total) * 100;
        
        printf("Movie: %s\n", shows[i].title);
        printf("  Total: %d | Booked: %d | Available: %d\n", total, booked, total - booked);
        printf("  Occupancy: %.2f%%\n\n", perc);
    }
}

// 5. File Handling Functions
void saveToFile() {
    FILE *fp = fopen(DATA_FILE, "wb");
    fwrite(shows, sizeof(Show), MAX_SHOWS, fp);
    fclose(fp);
}

void loadFromFile() {
    FILE *fp = fopen(DATA_FILE, "rb");
    if(!fp) {
        // Initialize default data if file doesn't exist
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
