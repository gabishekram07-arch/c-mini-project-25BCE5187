#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ROWS 5
#define COLS 10
#define MAX_SHOWS 3
#define DATA_FILE "shows.bin"
#define BOOKING_FILE "booking.bin"

typedef struct{
    char title[50];
    int seats[ROWS][COLS];
    float price;
}Show;

typedef struct{
    int bookingID;
    char name[50];
    int showIndex;
    int seats;
    char seatList[100];
    float total;
}Booking;

Show shows[MAX_SHOWS];

void loadData();
void saveData();
void displayMenu();
void displaySeats(int show);
void bookTicket();
void viewBooking();
void occupancyReport();
int parseSeat(char *input,int *r,int *c);
void trimnewline(char *str);

void trimnewline(char *str){
    str[strcspn(str,"\n")] = 0;
}

void displayMenu(){

    printf("\n===== MOVIE TICKET SYSTEM =====\n");
    printf("1. View Shows & Seats\n");
    printf("2. Book Ticket\n");
    printf("3. View Booking by ID\n");
    printf("4. Occupancy Report\n");
    printf("5. Exit\n");
}

int parseSeat(char *input,int *r,int *c){

    if(strlen(input)<2) return 0;

    *r = input[0]-'A';
    *c = atoi(&input[1])-1;

    if(*r>=0 && *r<ROWS && *c>=0 && *c<COLS)
        return 1;

    return 0;
}

void displaySeats(int show){

    printf("\n--- %s ---\n",shows[show].title);

    printf("    ");
    for(int i=1;i<=COLS;i++)
        printf("%2d ",i);

    printf("\n");

    for(int r=0;r<ROWS;r++){

        printf("%c | ",'A'+r);

        for(int c=0;c<COLS;c++){

            if(shows[show].seats[r][c])
                printf("X  ");
            else
                printf(".  ");
        }

        printf("\n");
    }
}

void bookTicket(){

    int show;
    int seats;
    char name[50];
    char input[20];
    char seatList[100]="";
    int r,c;

    printf("Select Show (1-3): ");
    scanf("%d",&show);

    if(show<1 || show>MAX_SHOWS){
        printf("Invalid show\n");
        return;
    }

    show--;

    displaySeats(show);

    printf("Number of seats: ");
    scanf("%d",&seats);

    if(seats<=0 || seats>ROWS*COLS){
        printf("Invalid seat count\n");
        return;
    }

    printf("Customer name: ");
    scanf("%s",name);

    for(int i=0;i<seats;i++){

        printf("Enter seat (example A1): ");
        scanf("%s",input);

        if(parseSeat(input,&r,&c)){

            if(shows[show].seats[r][c]==0){

                shows[show].seats[r][c]=1;

                strcat(seatList,input);
                strcat(seatList," ");

            }
            else{

                printf("Seat already booked\n");
                i--;
            }
        }
        else{

            printf("Invalid seat\n");
            i--;
        }
    }

    Booking b;

    b.bookingID = rand()%9000+1000;
    strcpy(b.name,name);
    b.showIndex = show;
    b.seats = seats;
    strcpy(b.seatList,seatList);
    b.total = seats*shows[show].price;

    FILE *fp=fopen(BOOKING_FILE,"ab");

    if(fp){
        fwrite(&b,sizeof(b),1,fp);
        fclose(fp);
    }

    saveData();

    printf("\nBooking Successful\n");
    printf("Booking ID: %d\n",b.bookingID);
    printf("Total: %.2f\n",b.total);
}

void viewBooking(){

    int id;
    Booking b;
    int found=0;

    printf("Enter Booking ID: ");
    scanf("%d",&id);

    FILE *fp=fopen(BOOKING_FILE,"rb");

    if(!fp){
        printf("No bookings\n");
        return;
    }

    while(fread(&b,sizeof(b),1,fp)){

        if(b.bookingID==id){

            printf("\nCustomer: %s\n",b.name);
            printf("Movie: %s\n",shows[b.showIndex].title);
            printf("Seats: %s\n",b.seatList);
            printf("Total: %.2f\n",b.total);

            found=1;
            break;
        }
    }

    fclose(fp);

    if(!found)
        printf("Booking not found\n");
}

void occupancyReport(){

    printf("\n=== OCCUPANCY REPORT ===\n");

    for(int i=0;i<MAX_SHOWS;i++){

        int booked=0;

        for(int r=0;r<ROWS;r++)
            for(int c=0;c<COLS;c++)
                if(shows[i].seats[r][c])
                    booked++;

        printf("%s : %d/%d booked\n",
        shows[i].title,booked,ROWS*COLS);
    }
}

void saveData(){

    FILE *fp=fopen(DATA_FILE,"wb");

    if(fp){
        fwrite(shows,sizeof(Show),MAX_SHOWS,fp);
        fclose(fp);
    }
}

void loadData(){

    FILE *fp=fopen(DATA_FILE,"rb");

    if(fp){
        fread(shows,sizeof(Show),MAX_SHOWS,fp);
        fclose(fp);
    }
    else{

        strcpy(shows[0].title,"Dune Part Two");
        shows[0].price=12;

        strcpy(shows[1].title,"Oppenheimer");
        shows[1].price=10;

        strcpy(shows[2].title,"The Batman");
        shows[2].price=11;

        for(int i=0;i<MAX_SHOWS;i++)
            for(int r=0;r<ROWS;r++)
                for(int c=0;c<COLS;c++)
                    shows[i].seats[r][c]=0;
    }
}

int main(){

    srand(time(NULL));
    loadData();

    int choice;

    while(1){

        displayMenu();

        printf("Enter choice: ");
        scanf("%d",&choice);

        switch(choice){

            case 1:
                printf("Select show (1-3): ");
                scanf("%d",&choice);

                if(choice>=1 && choice<=3)
                    displaySeats(choice-1);
                else
                    printf("Invalid show\n");
                break;

            case 2:
                bookTicket();
                break;

            case 3:
                viewBooking();
                break;

            case 4:
                occupancyReport();
                break;

            case 5:
                saveData();
                printf("Exiting...\n");
                exit(0);

            default:
                printf("Invalid choice\n");
        }
    }
}