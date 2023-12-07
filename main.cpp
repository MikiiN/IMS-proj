#include <simlib.h>
#include <iostream>

#define SECOND 1
#define MINUTE SECOND * 60
#define HOUR MINUTE * 60
#define DAY HOUR * 24

#define CASH_REGISTER_NUMBER 2

#define BEFORE_SHIFT 0
#define SHIFT 1

bool BrnoTrainArrive = false;
bool PragueTrainArrive = false;

Facility CashRegister[CASH_REGISTER_NUMBER];
Facility RailwayToBrno;
Facility RailwayToPraha;
Store TrainEntrance();
Queue TrainBrnoQueue("Brno Train");
Queue TrainPragueQueue("Prague Train");
Histogram Table("Table", 0, 1, 20);

using namespace std;

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
                Into(TrainBrnoQueue);
            }
            else{
                Into(TrainPragueQueue);
            }
        }
        else if(PragueTrainArrive){
            if(Random() < 0.1){
                Into(TrainBrnoQueue);
            }
            else{
                Into(TrainPragueQueue);
            }
        }
        else if(BrnoTrainArrive){
            if(Random() > 0.1){
                Into(TrainBrnoQueue);
            }
            else{
                Into(TrainPragueQueue);
            }
        }
        Passivate();

    }
};

class TrainToBrno : public Process {
    void Behavior() {
        BrnoTrainArrive = true;
        Wait(30 * MINUTE);
        Seize(RailwayToBrno);
        // TODO add 5 minute timer
        Wait(Uniform(MINUTE, MINUTE + 30 * SECOND));
        while(TrainBrnoQueue.Length() > 0){
            (TrainBrnoQueue.GetFirst())->Activate();
        }
        Release(RailwayToBrno);
        BrnoTrainArrive = false;
    }
};

class TrainToPrague : public Process {
    void Behavior() {
        PragueTrainArrive = true;
        Wait(30 * MINUTE);
        Seize(RailwayToPraha);
        // TODO add 5 minute timer
        Wait(Uniform(MINUTE, MINUTE + 30 * SECOND));
        Release(RailwayToPraha);
        PragueTrainArrive = false;
    }
};

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
            TrainBrnoQueue.clear();
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
            cout << "-------------------" << endl;
            TrainPragueQueue.clear();
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