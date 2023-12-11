#include <simlib.h>
#include <iostream>
#include <string>
#include <sstream>

#define SECOND 1
#define MINUTE SECOND * 60
#define HOUR MINUTE * 60
#define DAY HOUR * 24

#define FULL_TRAIN_CAPACITY 232

#define CASH_REGISTER_NUMBER 2

#define WAGON_NUMBER 5

#define BEFORE_SHIFT 0
#define SHIFT 1

bool BrnoTrainArrive = false;
bool PragueTrainArrive = false;

bool noOtherBrnoTrain = false;
bool noOtherPragueTrain = false;

int BrnoFreeTrainCapacity;
int PragueFreeTrainCapacity;

int PassengerMultiplier = 1;

Facility CashRegister[CASH_REGISTER_NUMBER];
Facility RailwayToBrno;
Facility RailwayToPrague;
Store BrnoTrainEntrance("Brno Train Entrance", 2*WAGON_NUMBER);
Store PragueTrainEntrance("Prague Train Entrance", 2*WAGON_NUMBER);
Queue TrainBrnoQueue("Brno Train");
Queue TrainPragueQueue("Prague Train");

using namespace std;

void printTrainTime(){
    int t = (int) Time;
    ostringstream str;
    str << "Train in " << t / 3600 << ":" << (t / 60) % 60 << ":" << t % 60 << endl;
    Print(str.str().data());
}

class TrainToBrno;
class TrainToPrague;

class TrainTimeout : public Event {
public:
    TrainTimeout(TrainToBrno *train) : Event() , BrnoTrain(train) {
    };
    TrainTimeout(TrainToPrague *train) : Event() , PragueTrain(train) {
    };
    
    void Behavior();
    
    TrainToPrague *PragueTrain = NULL;
    TrainToBrno *BrnoTrain = NULL;
};

class Passenger : public Process {
    void Behavior(){
        if (Random() < 0.40) {
            buyTicket();
        }
        
        double rnd = Random();
        bool flag;
        if(PragueTrainArrive || BrnoTrainArrive){
            // Prague or Brno train will arrive -> more passengers waiting
            // for them
            flag = rnd > 0.16;
        }
        else{
            flag = rnd > 0.4;
        }

        if (!flag){ // other trains
            return;
        }
        if(PragueTrainArrive && BrnoTrainArrive){
            if(Random() < 0.5){
                getToBrnoTrain();
            }
            else{
                getToPragueTrain();
            }
        }
        else if(PragueTrainArrive){
            if(Random() < 0.1){
                getToBrnoTrain();
            }
            else{
                getToPragueTrain();
            }
        }
        else if(BrnoTrainArrive){
            if(Random() > 0.1){
                getToBrnoTrain();
            }
            else{
                getToPragueTrain();
            }
        }
    }

    void buyTicket(){
        int index = -1;
        for (int i = 0; i < CASH_REGISTER_NUMBER; i++){
            if (!CashRegister[i].Busy()){
                index = i;
                break;
            }
        }
        if(index == -1){
            // passenger choose shorter queue
            if(CashRegister[0].QueueLen() < CashRegister[1].QueueLen()){
                index = 0;
            }
            else if(CashRegister[0].QueueLen() == CashRegister[1].QueueLen()){
                if(Random() < 0.5){
                    index = 0;
                }
                else{
                    index = 1;
                }
            }
            else{
                index = 1;
            }
        }
        Seize(CashRegister[index]);
        Wait(Uniform(25 * SECOND, 50 * SECOND));
        Release(CashRegister[index]);
    }

    void getToBrnoTrain(){
        if(noOtherBrnoTrain || RailwayToBrno.Busy()){
            return;
        }
        Into(TrainBrnoQueue);
        Passivate();

        BrnoFreeTrainCapacity--;
        if(BrnoFreeTrainCapacity == 0){
            Into(TrainBrnoQueue);
            Passivate();
        }
        
        Enter(BrnoTrainEntrance, 1);
        Wait(Uniform(3*SECOND, 5*SECOND));
        Leave(BrnoTrainEntrance, 1);
    }

    void getToPragueTrain(){
        if(noOtherPragueTrain || RailwayToPrague.Busy()){
            return;
        }
        Into(TrainPragueQueue);
        Passivate();

        PragueFreeTrainCapacity--;
        if(PragueFreeTrainCapacity <= 0){
            Into(TrainPragueQueue);
            Passivate();
        }

        Enter(PragueTrainEntrance, 1);
        Wait(Uniform(3*SECOND, 5*SECOND));
        Leave(PragueTrainEntrance, 1);
    }
};

class TrainToBrno : public Process {
    public:
    void Behavior() {
        BrnoTrainArrive = true;
        Wait(30 * MINUTE);
        Seize(RailwayToBrno);
        TrainTimeout *timer = new TrainTimeout(this);
        timer->Activate(Time + 5*MINUTE);
        Wait(Uniform(MINUTE, MINUTE + 30 * SECOND));
        BrnoFreeTrainCapacity = ((int) (Uniform(0.5, 0.7) * FULL_TRAIN_CAPACITY));
        while(TrainBrnoQueue.Length() > 0){
            (TrainBrnoQueue.GetFirst())->Activate();
        }
        Passivate();
        TrainBrnoQueue.Output();
        TrainBrnoQueue.clear();
        Release(RailwayToBrno);
        BrnoTrainArrive = false;
    }

    void Timeout(){
        this->Activate();
    }
};

class TrainToPrague : public Process {
    public:
    void Behavior() {
        PragueTrainArrive = true;
        Wait(30 * MINUTE);
        Seize(RailwayToPrague);
        printTrainTime();
        TrainTimeout *timer = new TrainTimeout(this);
        timer->Activate(Time + 5*MINUTE);
        Wait(Uniform(MINUTE, MINUTE + 30 * SECOND));
        PragueFreeTrainCapacity = ((int) (Uniform(0.5, 0.7) * FULL_TRAIN_CAPACITY));
        while(TrainPragueQueue.Length() > 0){
            (TrainPragueQueue.GetFirst())->Activate();
        }
        Passivate();
        TrainPragueQueue.Output();
        TrainPragueQueue.Clear();
        Release(RailwayToPrague);
        PragueTrainArrive = false;
    }

    void Timeout(){
        this->Activate();
    }
};

void TrainTimeout::Behavior() {
    if(PragueTrain != NULL){
        PragueTrain->Timeout();
    }
    else{
        BrnoTrain->Timeout();
    }
    Cancel();
}

class GeneratorPassenger : public Event {
    void Behavior() {
        (new Passenger)->Activate();
        if(BrnoTrainArrive && PragueTrainArrive){
            Activate(Time + Uniform(1 * SECOND, 30 * SECOND) / PassengerMultiplier);
        }
        else if(BrnoTrainArrive || PragueTrainArrive){
            Activate(Time + Uniform(1 * SECOND, 1 * MINUTE) / PassengerMultiplier);
        }
        else{
            Activate(Time + Uniform(MINUTE, 2 * MINUTE));
        }
    }
};

/**
 * 5:01
 * 6:01
 * 8:01
 * 10:01
 * 12:01
 * 14:01
 * 16:01
 * 17:01
 * 18:01
 * 20:01
 */
class GeneratorTrainToBrno : public Event {
    int counter = 0;
    void Behavior() {
        if(counter < 10){
            (new TrainToBrno)->Activate();
            counter++;
            if (counter == 1 || counter == 7 || counter == 8) {
                Activate(Time + HOUR);
            }
            else {
                Activate(Time + 2 * HOUR);
            }
        }
        else{
            noOtherBrnoTrain = true;
        }
    }
};

/**
 * 5:01
 * 6:01
 * 7:01
 * 8:01
 * 10:01
 * 12:01
 * 14:01
 * 16:01
 * 17:01
 * 18:01
 * 20:01
 */
class GeneratorTrainToPrague : public Event {
    int counter = 0;
    void Behavior() {
        if(counter < 11){
            (new TrainToPrague)->Activate();
            counter++;
            if ((counter >= 1 && counter <= 3) || counter == 8 || counter == 9) {
                Activate(Time + HOUR);
            }
            else {
                Activate(Time + 2 * HOUR);
            }
        }
        else{
            noOtherPragueTrain = true;
        }
    }
};

class GeneratorShift : public Event {
    int ShiftState = BEFORE_SHIFT;
    void Behavior() {
        if(ShiftState == BEFORE_SHIFT){
            ShiftState = SHIFT;
            Activate(Time + 4 * HOUR + 30 * MINUTE);
        }
        else if(ShiftState == SHIFT){
            (new GeneratorTrainToBrno)->Activate();
            (new GeneratorTrainToPrague)->Activate();
            (new GeneratorPassenger)->Activate();
        }
    }
};

int main(int argc, char *argv[]) {
    if(argc > 1){
        string arg = argv[1];
        if(!arg.compare("exp2")){
            Print(" Station model - experiment 2(double passenger rate)\n");
            SetOutput("modelStationExp2.out");
            PassengerMultiplier = 2;
        }
        else if(!arg.compare("exp3")){
            Print(" Station model - experiment 3(find system weakness)\n");
            SetOutput("modelStationExp3.out");
            PassengerMultiplier = 3;
        }
        else{
            Print(" Station model - normal\n");
            SetOutput("modelStationExp1.out");
        }
    }
    else{
        Print(" Station model - normal\n");
        SetOutput("modelStationExp1.out");
    }

    Init(0, (20*HOUR + 10*MINUTE));
    (new GeneratorShift)->Activate();
    Run();
    for (int i = 0; i < CASH_REGISTER_NUMBER; i++){
        CashRegister[i].Output();
    }
    return 0;
}