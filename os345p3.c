// os345p3.c - Jurassic Park
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "os345.h"
#include "os345park.h"
#include "dclock.h"

// ***********************************************************************
// project 3 variables

// Represents a "pipe" signaling something going in and out.
// to receive, signal `from`, then wait for `to`
// to send, wait for `from`, then signal `to`
typedef struct {
    // signalled when something can start going through the pipe
    Semaphore *from;
    // signalled when the thing has gone through the pipe
    Semaphore *to;
} SemaphorePipe;

typedef struct {
    int id;
    Semaphore *event;
} *SemaphoreAndId;

#define PARK_STATE(statement) do { SEM_WAIT(parkMutex); SWAP; statement; SWAP; SEM_SIGNAL(parkMutex); SWAP; } while (0)

#define SEM_INIT(sem, type, initial) do { sem = createSemaphore(#sem, (type), (initial)); } while (0)
#define SEMPIPE_INIT(pipe, type, initial) do { \
     (pipe).from  = createSemaphore(#pipe "_from", (type), (initial)); \
     (pipe).to = createSemaphore(#pipe "_to", (type), (initial)); \
     } while (0)

// Jurassic Park
extern JPARK myPark;
extern Semaphore *parkMutex;                        // protect park access
extern Semaphore *fillSeat[NUM_CARS];            // (signal) seat ready to fill
extern Semaphore *seatFilled[NUM_CARS];        // (wait) passenger seated
extern Semaphore *rideOver[NUM_CARS];            // (signal) ride over

static DClock dClock;
static SemaphorePipe passengerToCar;
static SemaphorePipe driverToCar;
static SemaphorePipe purchasingTicket;

static Semaphore *ticketsLeft;
static Semaphore *spaceInPark;
static Semaphore *spaceInMuseum;
static Semaphore *spaceInGiftShop;

static Semaphore *buyTicketMutex;
static Semaphore *sellTicketMutex;

// Inter-process communication
static void *eventData;
static Semaphore *eventDataCanBeSet;
static Semaphore *eventDataReady;
static Semaphore *eventDataReceived;
static Semaphore *eventThatWasSent;

extern Semaphore *tics10thsec;

static void passEventData(void *data, Semaphore *event);

static void *receiveEventData(Semaphore *event);

static void sendIntoPipeWithData(SemaphorePipe *pair, void *data);

static void receiveFromPipe(SemaphorePipe *pair);

static void *receiveFromPipeWithData(SemaphorePipe *pair);

static void waitForRandomTime(int maxTicks, Semaphore *event);

static Semaphore *waitForEither(Semaphore *sem1, Semaphore *sem2);

// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char **);

void CL3_dc(int, char **);

static int dClockTask(int argc, char *argv[]);

static int carTask(int argc, char *argv[]);

static int visitorTask(int argc, char *argv[]);

static int driverTask(int argc, char *argv[]);


// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char *argv[]) {
    char buf[32], buf2[32];
    char *newArgv[2];
    srand((unsigned int) time(0));

    // start park
    sprintf(buf, "jurassicPark");
    newArgv[0] = buf;
    createTask(buf,                // task name
               jurassicTask,                // task
               MED_PRIORITY,                // task priority
               1,                                // task count
               newArgv);                    // task argument

    SEM_INIT(ticketsLeft, COUNTING, MAX_TICKETS);
    SEM_INIT(spaceInPark, COUNTING, MAX_IN_PARK);
    SEM_INIT(spaceInMuseum, COUNTING, MAX_IN_MUSEUM);
    SEM_INIT(spaceInGiftShop, COUNTING, MAX_IN_GIFTSHOP);

    SEMPIPE_INIT(passengerToCar, BINARY, 0);
    SEMPIPE_INIT(driverToCar, COUNTING, 0);
    SEMPIPE_INIT(purchasingTicket, COUNTING, 0);

    SEM_INIT(eventDataCanBeSet, BINARY, 1);
    SEM_INIT(buyTicketMutex, BINARY, 1);
    SEM_INIT(sellTicketMutex, BINARY, 1);
    SEM_INIT(eventDataReady, BINARY, 0);
    SEM_INIT(eventDataReceived, BINARY, 0);
    SEM_INIT(eventThatWasSent, BINARY, 0);


    // wait for park to get initialized...
    while (!parkMutex) SWAP;
    // wait for the delta clock to be cleared
    while (semTryLock(tics10thsec));

    printf("\nStart Jurassic Park...");
    dClock = initDClock("Jurassic");
    SWAP;
    newArgv[0] = "dClock";
    SWAP;
    createTask("dClock", dClockTask, MED_PRIORITY, 1, newArgv);
    SWAP;

    for (int i = 0; i < NUM_CARS; ++i) {
        sprintf(buf, "Car_%02d", i);
        SWAP;
        sprintf(buf2, "%d", i);
        SWAP;
        newArgv[0] = buf, newArgv[1] = buf2;
        SWAP;
        createTask(buf, carTask, MED_PRIORITY, 2, newArgv);
        SWAP;
    }

    for (int i = 0; i < NUM_DRIVERS; ++i) {
        sprintf(buf, "Driver_%02d", i);
        SWAP;
        sprintf(buf2, "%d", i);
        SWAP;
        newArgv[0] = buf, newArgv[1] = buf2;
        SWAP;
        createTask(buf, driverTask, MED_PRIORITY, 2, newArgv);
        SWAP;
    }

    for (int i = 0; i < NUM_VISITORS; ++i) {
        sprintf(buf, "Visitor_%02d", i);
        SWAP;
        sprintf(buf2, "%d", i);
        SWAP;
        newArgv[0] = buf, newArgv[1] = buf2;
        SWAP;
        createTask(buf, visitorTask, MED_PRIORITY, 2, newArgv);
        SWAP;
    }


    return 0;
} // end project3



// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char *argv[]) {
    printDClock(dClock, argc > 1);
    return 0;
} // end CL3_dc

int dClockTask(int argc, char **argv) {
    while (1) {
        int ticks = 0;
        SWAP;
        while (semTryLock(tics10thsec)) {
            ++ticks;
            SWAP;
        }
        tickDClock(dClock, ticks);
        SWAP;
    }
    return 0;
}

static int carTask(int argc, char *argv[]) {
    assert(argc == 2);
    int carId = atoi(argv[1]);
    Semaphore *passengerFinishedEvent[NUM_SEATS];
    Semaphore *driverFinishedEvent;

    do {
        for (int i = 0; i < NUM_SEATS; ++i) {
            // Wait for a seat that is ready to be filled
            SEM_WAIT(fillSeat[carId]);
            SWAP;

            // Receive a visitor and get the semaphore to signal when we're done
            passengerFinishedEvent[i] = receiveFromPipeWithData(&passengerToCar);
            SWAP;

            // Tell the park that a visitor's now in this seat
            SEM_SIGNAL(seatFilled[carId]);
            SWAP;
        }

        // Get an available driver
        SemaphoreAndId driverData = receiveFromPipeWithData(&driverToCar);
        SWAP;
        PARK_STATE(myPark.drivers[driverData->id] = carId + 1);
        SWAP;
        driverFinishedEvent = driverData->event;
        SWAP;
        free(driverData);
        SWAP;

        // Wait until the ride has finished
        SEM_WAIT(rideOver[carId]);
        SWAP;

        // Let the driver continue with their life
        assert(driverFinishedEvent);
        SEM_SIGNAL(driverFinishedEvent);
        SWAP;
        driverFinishedEvent = NULL;
        SWAP;

        // Don't keep the passengers prisoner
        for (int i = 0; i < NUM_SEATS; ++i) {
            assert(passengerFinishedEvent[i]);
            SEM_SIGNAL(passengerFinishedEvent[i]);
            SWAP;
            passengerFinishedEvent[i] = NULL;
            SWAP;
        }
    } while (myPark.numExitedPark < NUM_VISITORS);

    return 0;
}

static int visitorTask(int argc, char *argv[]) {
    assert(argc == 2);
    char buf[30];
    int visitorId = atoi(argv[1]);
    SWAP;
    sprintf(buf, "visitor %02d", visitorId);
    SWAP;
    Semaphore *finishedWaiting = createSemaphore(buf, BINARY, 0);
    SWAP;

    waitForRandomTime(100, finishedWaiting);
    SWAP;
    // enter the park
    PARK_STATE(++myPark.numOutsidePark);
    SEM_WAIT(spaceInPark);
    SWAP;

    // enter the ticket line
    PARK_STATE(
            --myPark.numOutsidePark;
            SWAP;
            ++myPark.numInPark;
            SWAP;
            ++myPark.numInTicketLine;
    );

    waitForRandomTime(30, finishedWaiting);
    SWAP;
    SEM_WAIT(buyTicketMutex);
    SWAP;
    // buy a ticket
    receiveFromPipe(&purchasingTicket);
    SWAP;
    // now waiting to go into the museum
    PARK_STATE(
            --myPark.numTicketsAvailable;
            SWAP;
            --myPark.numInTicketLine;
            SWAP;
            ++myPark.numInMuseumLine;
    );
    SEM_SIGNAL(buyTicketMutex);
    SWAP;

    // wait to actually go into the museum
    SEM_WAIT(spaceInMuseum);
    SWAP;
    // wait to enter the museum
    waitForRandomTime(30, finishedWaiting);
    SWAP;

    PARK_STATE(
            --myPark.numInMuseumLine;
            SWAP;
            ++myPark.numInMuseum;
    );

    // wander around the museum
    waitForRandomTime(30, finishedWaiting);

    // get rid of our ticket
    SEM_SIGNAL(spaceInMuseum);
    SWAP;

    // move into the tour line
    PARK_STATE(
            --myPark.numInMuseum;
            SWAP;
            ++myPark.numInCarLine;
    );

    waitForRandomTime(30, finishedWaiting);
    SWAP;
    sendIntoPipeWithData(&passengerToCar, finishedWaiting);
    SWAP;
    PARK_STATE(
            --myPark.numInCarLine;
            SWAP;
            ++myPark.numInCars;
            SWAP;
            ++myPark.numTicketsAvailable;
            SWAP;
            SEM_SIGNAL(ticketsLeft);
    );
    // Wait for us to finish the ride
    SEM_WAIT(finishedWaiting);
    SWAP;

    // move to the gift shop line
    PARK_STATE(
            --myPark.numInCars;
            SWAP;
            ++myPark.numInGiftLine;
    );
    waitForRandomTime(30, finishedWaiting);
    SWAP;
    // enter the gift shop
    SEM_WAIT(spaceInGiftShop);
    SWAP;

    PARK_STATE(
            --myPark.numInGiftLine;
            SWAP;
            ++myPark.numInGiftShop;
    );

    // spend some time window shopping
    waitForRandomTime(30, finishedWaiting);
    SWAP;

    // leave the gift shop
    SEM_SIGNAL(spaceInGiftShop);
    SWAP;

    // leave the park
    PARK_STATE(
            --myPark.numInPark;
            SWAP;
            --myPark.numInGiftShop;
            SWAP;
            ++myPark.numExitedPark;
    );

    // there's more space now
    SEM_SIGNAL(spaceInPark);
    SWAP;
    deleteSemaphore(&finishedWaiting);
    SWAP;
    return 0;
}

int driverTask(int argc, char **argv) {
    assert(argc == 2);
    char buf[32];
    int driverId = atoi(argv[1]);
    SWAP;
    sprintf(buf, "driver %d", driverId);
    SWAP;
    Semaphore *finishedDriving = createSemaphore(buf, BINARY, 0);
    SWAP;

    Semaphore *event;
    do {
        event = waitForEither(driverToCar.from, purchasingTicket.from);
        SWAP;
        if (event == driverToCar.from) {
            // this driver needs to drive a car
            SemaphoreAndId driverData;
            driverData = malloc(sizeof(*driverData));
            assert(driverData);
            SWAP;
            driverData->id = driverId;
            SWAP;
            driverData->event = finishedDriving;
            SWAP;
            passEventData(driverData, driverToCar.to);
            SWAP;
            SEM_WAIT(finishedDriving);
            SWAP;
            PARK_STATE(myPark.drivers[driverId] = 0);
            SWAP;
        } else if (event == purchasingTicket.from) {
            SEM_WAIT(sellTicketMutex);
            PARK_STATE(myPark.drivers[driverId] = -1);
            SWAP;
            // this driver needs to provide a ticket
            SEM_WAIT(ticketsLeft);
            SWAP;
            PARK_STATE(myPark.drivers[driverId] = 0);
            SWAP;
            SEM_SIGNAL(purchasingTicket.to);
            SWAP;
            SEM_SIGNAL(sellTicketMutex);
        }
    } while (myPark.numExitedPark < NUM_VISITORS);
    SWAP;
    deleteSemaphore(&finishedDriving);
    SWAP;
    return 0;
}

void waitForRandomTime(int maxTicks, Semaphore *event) {
    semTryLock(event);
    SWAP;
    insertDClock(dClock, rand() % maxTicks, event);
    SWAP;
    SEM_WAIT(event);
    SWAP;
}

void passEventData(void *data, Semaphore *event) {
    SEM_WAIT(eventDataCanBeSet);
    SWAP;
    eventData = data;
    SWAP;
    eventThatWasSent = event;
    SWAP;
    SEM_SIGNAL(eventDataReady);
    SWAP;
    SEM_SIGNAL(event);
    SWAP;
    SEM_WAIT(eventDataReceived);
    SWAP;
    SEM_SIGNAL(eventDataCanBeSet);
    SWAP;
}

void *receiveEventData(Semaphore *event) {
    SEM_WAIT(event);
    SWAP;

    while (1) {
        SEM_WAIT(eventDataReady);
        SWAP;
        if (eventThatWasSent != event) {
            SEM_SIGNAL(eventDataReady);
            SWAP;
        } else {
            SWAP;
            break;
        }
    }
    void *data = eventData;
    SWAP;
    SEM_SIGNAL(eventDataReceived);
    SWAP;
    return data;
}

void sendIntoPipeWithData(SemaphorePipe *pair, void *data) {
    SEM_WAIT(pair->from);
    SWAP;
    passEventData(data, pair->to);
}

void *receiveFromPipeWithData(SemaphorePipe *pair) {
    SEM_SIGNAL(pair->from);
    SWAP;
    return receiveEventData(pair->to);
}

void receiveFromPipe(SemaphorePipe *pair) {
    SEM_SIGNAL(pair->from);
    SWAP;
    SEM_WAIT(pair->to);
    SWAP;
}

Semaphore *waitForEither(Semaphore *sem1, Semaphore *sem2) {
    while (1) {
        if (semTryLock(sem1)) {
            SWAP;
            return sem1;
        }
        SWAP;
        if (semTryLock(sem2)) {
            SWAP;
            return sem2;
        }
        SWAP;
    }
}

/*
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printf("\nDelta Clock");
	// ?? Implement a routine to display the current delta clock contents
	//printf("\nTo Be Implemented!");
	int i;
	for (i=0; i<numDeltaClock; i++)
	{
		printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
	}
	return 0;
} // end CL3_dc


// ***********************************************************************
// display all pending events in the delta clock list
void printDeltaClock(void)
{
	int i;
	for (i=0; i<numDeltaClock; i++)
	{
		printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
	}
	return;
}


// ***********************************************************************
// test delta clock
int P3_tdc(int argc, char* argv[])
{
	createTask( "DC Test",			// task name
		dcMonitorTask,		// task
		10,					// task priority
		argc,					// task arguments
		argv);

	timeTaskID = createTask( "Time",		// task name
		timeTask,	// task
		10,			// task priority
		argc,			// task arguments
		argv);
	return 0;
} // end P3_tdc



// ***********************************************************************
// monitor the delta clock task
int dcMonitorTask(int argc, char* argv[])
{
	int i, flg;
	char buf[32];
	// create some test times for event[0-9]
	int ttime[10] = {
		90, 300, 50, 170, 340, 300, 50, 300, 40, 110	};

	for (i=0; i<10; i++)
	{
		sprintf(buf, "event[%d]", i);
		event[i] = createSemaphore(buf, BINARY, 0);
		insertDeltaClock(ttime[i], event[i]);
	}
	printDeltaClock();

	while (numDeltaClock > 0)
	{
		SEM_WAIT(dcChange)
		flg = 0;
		for (i=0; i<10; i++)
		{
			if (event[i]->state ==1)			{
					printf("\n  event[%d] signaled", i);
					event[i]->state = 0;
					flg = 1;
				}
		}
		if (flg) printDeltaClock();
	}
	printf("\nNo more events in Delta Clock");

	// kill dcMonitorTask
	tcb[timeTaskID].state = S_EXIT;
	return 0;
} // end dcMonitorTask


extern Semaphore* tics1sec;

// ********************************************************************************************
// display time every tics1sec
int timeTask(int argc, char* argv[])
{
	char svtime[64];						// ascii current time
	while (1)
	{
		SEM_WAIT(tics1sec)
		printf("\nTime = %s", myTime(svtime));
	}
	return 0;
} // end timeTask
*/

