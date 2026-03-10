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

Show shows[MAX_SHOWS];

static int show_menu = 1;
static int sub_state = 0;
static int selected_show = -1;
static int seats_to_book = 0;

void initializeShows();
void displaySeats(int showIdx);
void showOccupancyReport();
void saveToFile();
void loadFromFile();

/* ------------------------------------------------ */
/* MAIN LOOP (browser friendly)                     */
/* ------------------------------------------------ */

void main_loop() {

    if (show_menu) {
        printf("\n--- MOVIE TICKET SYSTEM ---\n");
        printf("1. View Shows & Seats\n");
        printf("2. Book Tickets\n");
        printf("3. Occupancy Report\n");
        printf("4. Exit\n");
        printf("Enter choice: ");
        fflush(stdout);

        show_menu = 0;
    }

    char buffer[100];

    if (fgets(buffer, sizeof(buffer), stdin)) {

        int num = atoi(buffer);

        if (sub_state == 0) {

            switch (num) {

                case 1:
                    printf("\nEnter Show Number (1-3): ");
                    fflush(stdout);
                    sub_state = 1;
                    break;

                case 2:
                    printf("\nSelect Show (1-3): ");
                    fflush(stdout);
                    sub_state = 2;
                    break;

                case 3:
                    showOccupancyReport();
                    show_menu = 1;
                    break;

                case 4:
                    saveToFile();
                    printf("\nGoodbye! Data saved.\n");
                    emscripten_cancel_main_loop();
                    return;

                default:
                    printf("Invalid choice.\n");
                    show_menu = 1;
            }
        }

        else if (sub_state == 1) {

            int s = num - 1;

            if (s >= 0 && s < MAX_SHOWS) {
                displaySeats(s);
            } else {
                printf("Invalid show number.\n");
            }

            sub_state = 0;
            show_menu = 1;
        }

        else if (sub_state == 2) {

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
                show_menu = 1;
            }
        }

        else if (sub_state == 3) {

            seats_to_book = num;

            if (seats_to_book < 1 || seats_to_book > ROWS * COLS) {
                printf("Invalid number of seats.\n");
            } else {
                printf("Booking %d seats for %s\n",
                       seats_to_book,
                       shows[selected_show].title);
            }

            sub_state = 0;
            show_menu = 1;
        }
    }
}

/* ------------------------------------------------ */
/* MAIN                                             */
/* ------------------------------------------------ */

int main() {

    srand(time(NULL));

    loadFromFile();

    printf("Movie Ticket System loaded.\n");

    emscripten_set_main_loop(main_loop, 0, 1);

    return 0;
}

/* ------------------------------------------------ */
/* DISPLAY SEATS                                    */
/* ------------------------------------------------ */

void displaySeats(int showIdx) {

    printf("\n--- %s ---\n", shows[showIdx].title);

    printf("    ");
    for (int j = 1; j <= COLS; j++)
        printf("%2d ", j);

    printf("\n");

    for (int i = 0; i < ROWS; i++) {

        printf("%c | ", 'A' + i);

        for (int j = 0; j < COLS; j++) {

            if (shows[showIdx].seats[i][j])
                printf("X  ");
            else
                printf(".  ");
        }

        printf("\n");
    }

    printf(". = available   X = booked\n\n");

    fflush(stdout);
}

/* ------------------------------------------------ */
/* OCCUPANCY REPORT                                 */
/* ------------------------------------------------ */

void showOccupancyReport() {

    printf("\n--- OCCUPANCY REPORT ---\n");

    for (int i = 0; i < MAX_SHOWS; i++) {

        int booked = 0;

        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (shows[i].seats[r][c])
                    booked++;

        printf("%-20s  Booked: %2d / %d   (%.0f%%)\n",
               shows[i].title,
               booked,
               ROWS * COLS,
               (booked * 100.0) / (ROWS * COLS));
    }

    printf("\n");
}

/* ------------------------------------------------ */
/* FILE SAVE                                        */
/* ------------------------------------------------ */

void saveToFile() {

    FILE *fp = fopen(DATA_FILE, "wb");

    if (fp) {
        fwrite(shows, sizeof(Show), MAX_SHOWS, fp);
        fclose(fp);
    }
}

/* ------------------------------------------------ */
/* FILE LOAD                                        */
/* ------------------------------------------------ */

void loadFromFile() {

    FILE *fp = fopen(DATA_FILE, "rb");

    if (fp) {

        fread(shows, sizeof(Show), MAX_SHOWS, fp);

        fclose(fp);

    } else {

        strcpy(shows[0].title, "Dune: Part Two");
        shows[0].price = 12.50;

        strcpy(shows[1].title, "Oppenheimer");
        shows[1].price = 10.00;

        strcpy(shows[2].title, "The Batman");
        shows[2].price = 11.00;

        for (int i = 0; i < MAX_SHOWS; i++)
            for (int r = 0; r < ROWS; r++)
                for (int c = 0; c < COLS; c++)
                    shows[i].seats[r][c] = 0;
    }
}