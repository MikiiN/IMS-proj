#include <simlib.h>
#include <iostream>

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

int BrnoFreeTrainCapacity;
int PragueFreeTrainCapacity;

Facility CashRegister[CASH_REGISTER_NUMBER];
Facility RailwayToBrno;
Facility RailwayToPrague;
Store BrnoTrainEntrance("Brno Train Entrance", 2*WAGON_NUMBER);
Store PragueTrainEntrance("Prague Train Entrance", 2*WAGON_NUMBER);
Queue TrainBrnoQueue("Brno Train");
Queue TrainPragueQueue("Prague Train");
Histogram Table("Table", 0, 1, 20);

using namespace std;

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
        if (Random() < 0.45) {
            int index = -1;
            for (int i = 0; i < CASH_REGISTER_NUMBER; i++){
                if (!CashRegister[i].Busy()){
                    index = i;
                    break;
                }
            }
            if(index == -1){
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
            Wait(Uniform(40 * SECOND, MINUTE));
            Release(CashRegister[index]);
        }
        
        double rnd = Random();
        bool flag;
        if(PragueTrainArrive || BrnoTrainArrive){
            flag = rnd > 0.15;
        }
        else{
            flag = rnd > 0.4;
        }

        if (!flag){
            return;
        }
        if(PragueTrainArrive && BrnoTrainArrive){
            if(Random() < 0.5){
                addToBrnoQueue();
            }
            else{
                addToPragueQueue();
            }
        }
        else if(PragueTrainArrive){
            if(Random() < 0.1){
                addToBrnoQueue();
            }
            else{
                addToPragueQueue();
            }
        }
        else if(BrnoTrainArrive){
            if(Random() > 0.1){
                addToBrnoQueue();
            }
            else{
                addToPragueQueue();
            }
        }
    }

    void addToBrnoQueue(){
        Into(TrainBrnoQueue);
        Passivate();

        BrnoFreeTrainCapacity--;
        if(BrnoFreeTrainCapacity == 0){
            return;
        }

        Enter(BrnoTrainEntrance, 1);
        Wait(Uniform(3*SECOND, 5*SECOND));
        Leave(BrnoTrainEntrance, 1);
    }

    void addToPragueQueue(){
        Into(TrainPragueQueue);
        Passivate();

        PragueFreeTrainCapacity--;
        if(PragueFreeTrainCapacity == 0){
            return;
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
        // TODO add 5 minute timer
        TrainTimeout *timer = new TrainTimeout(this);
        timer->Activate(Time + 5*MINUTE);
        Wait(Uniform(MINUTE, MINUTE + 30 * SECOND));
        BrnoFreeTrainCapacity = FULL_TRAIN_CAPACITY - ((int) (Uniform(0.3, 0.4) * FULL_TRAIN_CAPACITY));
        while(TrainBrnoQueue.Length() > 0){
            (TrainBrnoQueue.GetFirst())->Activate();
        }
        Passivate();
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
        // TODO add 5 minute timer
        TrainTimeout *timer = new TrainTimeout(this);
        timer->Activate(Time + 5*MINUTE);
        Wait(Uniform(MINUTE, MINUTE + 30 * SECOND));
        PragueFreeTrainCapacity = FULL_TRAIN_CAPACITY - ((int) (Uniform(0.2, 0.3) * FULL_TRAIN_CAPACITY));
        while(TrainPragueQueue.Length() > 0){
            (TrainPragueQueue.GetFirst())->Activate();
        }
        Passivate();
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
            Activate(Time + Exponential(20 * SECOND));
        }
        else if(BrnoTrainArrive || PragueTrainArrive){
            Activate(Time + Exponential(40 * SECOND));
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

int main() {
    Print(" test - SIMLIB/C++ example\n");
    SetOutput("test.out");
    Init(0, DAY);
    (new GeneratorShift)->Activate();
    Run();
    for (int i = 0; i < CASH_REGISTER_NUMBER; i++){
        CashRegister[i].Output();
    }
    TrainBrnoQueue.Output();
    TrainPragueQueue.Output();
    Table.Output();
    return 0;
}