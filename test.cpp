#include "simlib.h"
#include <ctime>

//minuta
const int MINUTA = 60;
//hodina
const int HODINA = 60*MINUTA;
//den
const int DEN = 24*HODINA;
//mesic
const int MESIC = 30*DEN;
//rok
const int ROK = 12*MESIC;

//doba po kterou simulace bezi
const double DOBA_SIMULACE = 1*ROK;

//kapacita letadla
const int KAPACITA_LETADLA =170;
//pocet prepazek pro checkin
const int CHECKINU = 8;
//pocet otevyranich checkinu pri priletu letadla
const int OTEVRIT_CHECKINU = 2;
//pocet bran na odbaveni kufru
const int PRILET_BRAN = 2;
//pocet bran pro odlety
const int ODLET_BRAN = 7;
//max letadel pro nakladani cestujcich
const int ODLET_NAKLADANI = 3;

//pocet otevyranich bezpecnostnich ulicek pri priletu letadla
const int OTEVRIT_BEZP = 1;//NEMENIT
//pocet  bezpecnostnich kontrol
const int BEZPECNOST = 1; //NEMENIT
//pocet pasovych kontrol
const int PASU = 4;
//pocet mist pro odkladani obleceni
const int OBLECENI = 4;
//pocet bezpecnostich ramu
const int RAMU = 2;
//pocet stanovist pro osobni prohlidku
const int OSOBNI = 1;


//pocet otevrenych prepazek;
int checkin_otevrenych = 0;
//pocet vygenerovanych pozadavku na prepazky
int checkin_vygenerovanych = 0;
//otevrenych bezpecnostnich prepazek
int bezpec_otevrenych = 0;
//pocet vygenerovanych pozadavku na bezpecnost
int bezpec_vygenerovanych = 0;
//pocet volnych mist, kde muze letadlo vykladat
int prilet_otevrenych = PRILET_BRAN;
//pocet otevrenych odletovych bran
int odlet_ovetrenych = 0;
//pocet zabranych bran k nakladani
int odlet_nakladani = 0;

//stridani dne a noci
bool je_den = false;
//stridani sezon
bool je_sezona = true;

//prepazky pro checkin
Facility checkin[CHECKINU];
int pouzita[CHECKINU];
//fronta na checkin kam se skladuji lidi pokud neni otevren
Queue q_checkin;

//runway
Facility Runway;
//brany pro odbaveni kufru
Facility brana_prilet[PRILET_BRAN];
//pokud uz 2 letadla odebiraji kufry stojanka je vlastne fronta
Queue q_apron;

//odletove brany
Queue q_odlet_brany[ODLET_BRAN];
//fronta pro letadla na odletove brany
Queue q_odlet_brany_letadla;
//fronta na nakladaci brany
Queue q_odlet_nakladani;
//fronta u odletovych bran
Queue q_nakladani;

//pasova kontrola
Store pasova_kontrola("Pasy", PASU);
//bezpecnostni ramy
Store ramy("Ramy", RAMU);
//osobni prohlidka
Store osobni("Osobni", OSOBNI);
//odkladani obleceni
Store vyslekani("vyslekani",OBLECENI);
//oblekani
Store oblekani("Oblekani",OBLECENI);


//fronta na bezpecnostni prohlidku pokud je uzavrena
Queue q_bezp_hala;


//statistiky
Histogram odbaveni_zakaznika("Celkova doba zakazniku v systemu",  1000, 10*MINUTA, 20);
Histogram odbaveni_letadla("Celkova doba letadel systemu",1200, 5*MINUTA, 20);
Histogram cekani_bezp("Doba trvani bezpecnostni prohlidky", 0, 10*MINUTA, 15);

int pocet_prichozich_zakazniku = 0;
int pocet_odchozich_zakazniku = 0;
int pocet_in_letadel = 0;
int pocet_out_letadel = 0;


Stat bezp_fronta;
int lidi_na_bezp=0;


//udalost pravidelne kontrolujici letadla cekajici na nalozeni
//provede nalozeni letadla pokud je v nejakem gatu dostatek lidi
class Nalozeni_cestujicich : public Event {


	void Behavior() {

		//pokusit se nalozit vsechny letadla
		while( ! q_nakladani.Empty()) {

			bool nalozeno = false;

			//najit branu s dostatecnym poctem lidi
			for(int i=0; i < ODLET_BRAN; i++) {

				if(q_odlet_brany[i].Length() >= KAPACITA_LETADLA) {

					nalozeno = true;
					for(int j=0; j < KAPACITA_LETADLA; j++) {
						q_odlet_brany[i].GetFirst()->Activate();
					}
					break;
				}
			}

			//lidi nalozeni
			if(nalozeno) {
				(q_nakladani.GetFirst())->Activate();

			//nikde nejsou lidi
			} else {
				break;
			}


		}

		Activate(Time+5);
	}
};


//proces zakaznika cekajiciho na odbaveni
class Zakaznik : public Process {

	void Behavior() {

		double prichod = Time;
		pocet_prichozich_zakazniku++;

		//zakaznik jde na checkin
		if(Random() <= 0.35) {

			//zadny checkin neni otevreny, zakaznici cekaji
			if(checkin_otevrenych == 0) {
				q_checkin.Insert(this);
				Passivate();
			}

			int idx=0;
			for (int a=0; a < checkin_otevrenych; a++) {
				if (checkin[a].QueueLen() < checkin[idx].QueueLen())
					idx=a;
			}

			pouzita[idx]++;

			//zakaznik jde na checkin
			Seize(checkin[idx]);
			Wait(Uniform(30,60));
			Release(checkin[idx]);
		}

		//zadna bezpecnost neni otevrena, zakaznici cekaji
		if(bezpec_otevrenych == 0) {
			q_bezp_hala.Insert(this);
			Passivate();
		}

		//zakaznik jde na bezpecnostni prohlidku
		double pp = Time;

		lidi_na_bezp++;
		bezp_fronta(lidi_na_bezp);

		//35% lidi leti mimo schengen, musi projit pasovou kontorlou
		if(Random() <= 0.35) {				
			Enter(pasova_kontrola, 1);
			Wait(15);
			Leave(pasova_kontrola);
		}

		//odkladani kovovych veci
		Enter(vyslekani, 1);
		Wait(45);
		Leave(vyslekani);

		//tak dlouho se prochazi ramem az nepiska
		for(int i=0; projiti_ramem() && (i < 3); i++);

		//obleceni po prohlidce
		Enter(oblekani, 1);
		Wait(30);
		Leave(oblekani);

		lidi_na_bezp--;
		cekani_bezp(Time-pp);

		//zakaznik se presunuje do gatu
		Wait(Uniform(10,15));

		int idx=0;
		for (int i = 0; i < odlet_ovetrenych; ++i) {
			if(q_odlet_brany[i].Length() > q_odlet_brany[idx].Length()) {
				idx = i;
			}
		}
		q_odlet_brany[idx].Insert(this);
		Passivate();

		odbaveni_zakaznika(Time-prichod);
		pocet_odchozich_zakazniku++;
	}

	//projiti ramem a pripadna prohlidka
	//vraci true pokud ram piskal
	bool projiti_ramem() {

		Enter(ramy, 1);
		Wait(15);
		Leave(ramy);

		//ram piskal
		if(Random() <= 0.3) {

			//osobni prohlidka
			if(Random() < 0.5) {
				Enter(osobni, 1);
				Wait(15);
				Leave(osobni);
				return false;
			}

			return true;
		} else {
			return false;
		}

	}

};


//proces pro otevreni checkinu a bezpecnostni ulicky pri priletu letadla
class Otevrit : public Process {

	void Behavior() {

		//otevreni checkinu
		if(checkin_otevrenych < CHECKINU) {
			checkin_otevrenych += OTEVRIT_CHECKINU;
		}
		checkin_vygenerovanych += OTEVRIT_CHECKINU;

		//upozorneni cekajicich na otevreni prepazek
		while (!q_checkin.Empty()) {
			Process *tmp = (Process *)q_checkin.GetFirst();
			tmp->Activate();
		}


		//otevreni bezpecnosti
		if(bezpec_otevrenych < BEZPECNOST) {
			bezpec_otevrenych += OTEVRIT_BEZP;
		}
		bezpec_vygenerovanych += OTEVRIT_BEZP;

		//upozorneni cekajicich na otevreni bezpecnosti
		while (!q_bezp_hala.Empty()) {
			Process *tmp = (Process *)q_bezp_hala.GetFirst();
			tmp->Activate();
		}
	}
};

//proces uzavirani checkinu a bezpecnostni ulicky
class Uzavrit : public Process {

	void Behavior() {

		if(checkin_vygenerovanych <= CHECKINU) {
			checkin_otevrenych -= OTEVRIT_CHECKINU;
		}
		checkin_vygenerovanych -= OTEVRIT_CHECKINU;


		if(bezpec_vygenerovanych <= BEZPECNOST) {
			bezpec_otevrenych -= OTEVRIT_BEZP;
		}
		bezpec_vygenerovanych -= OTEVRIT_BEZP;

	}
};

//proces pro vylozeni zavazadel, dostane index zarizeni s nejnizsi frontou
//tu obsadi a vylozi zavazadla az bude moct
class Vykladani : public Process {
	int Cislo_brany;
	void Behavior(){

		prilet_otevrenych--;

		//provadeni vykladani zavazadel
		Seize(brana_prilet[Cislo_brany]);
		Wait (Uniform((10*MINUTA),(15*MINUTA)));
		Release(brana_prilet[Cislo_brany]);
		
		prilet_otevrenych++;

		//signal dalsimu letadlu ze je volno
		if (!q_apron.Empty()) {
			Process *tmp = (Process *)q_apron.GetFirst();
			tmp->Activate();
		}
		

		Wait (Uniform((20*MINUTA),(30*MINUTA)));
	}
	
	public:
		Vykladani (int Pristavaci_brana) : Cislo_brany(Pristavaci_brana) {
			Activate();
		}
};

//generátor cestujících, vstupním parametrem je pocet zakazniku k vygenerovani
class Gen_cestujicich : public Event {

	int zakazniku;
	int vygen;

	void Behavior() {

		int shluk = Uniform(30,40);

		(new Zakaznik)->Activate();
		vygen++;

		if(vygen < zakazniku) {
			Activate(Time + Uniform(15,40));

		}
		
	}

	public:
		Gen_cestujicich(int zak) : zakazniku(zak) {
			vygen = 0;
			Activate();
		}
};

class Letadlo : public Process {


	void Behavior() {

		pocet_in_letadel++;
		
		//generator lidi
		(new Gen_cestujicich(KAPACITA_LETADLA))->Activate();
		Wait(0.5*HODINA);


		//otevri prepazky
		(new Otevrit)->Activate();

		//neni volna odletova brana letadlo ceka
		if(odlet_ovetrenych >= ODLET_BRAN) {
			q_odlet_brany_letadla.Insert(this);
			Passivate();
		}

		odlet_ovetrenych++;
		
		//pockej 2 hodiny, jses na ceste letadlo
		Wait(1.5*HODINA);

		//pristani letadla		
		Seize(Runway);
		Wait (3*MINUTA);
		Release(Runway);

		//cas priletu letadla na finalu
		double prilet = Time;
		
		//fronta, pokud bude vic jak 3 letadla chtit odebirat zavazadla, tak si ostatni stoupnou do fronty
		if (prilet_otevrenych==0) {
			q_apron.Insert(this);
			Passivate();
		}
		//Maximalne 2 letadla se muzou odbavovat zaraz
		int idx=0;
		for (int a=0; a < prilet_otevrenych; a++) {
			if (brana_prilet[a].QueueLen() < brana_prilet[idx].QueueLen()) {
				idx=a;
			}
		}
		//paralerne vystupovani lidi a vykladani zavazadel
		(new Vykladani(idx))->Activate();
		
		//vystupovani cestujicich
		Wait(Uniform(7*MINUTA, 10*MINUTA));
		
		//uklid
		Wait(Uniform(10*MINUTA,40*MINUTA));

		//neni volna brana pro nakladani
		if(odlet_nakladani >= ODLET_NAKLADANI) {
			q_odlet_nakladani.Insert(this);
			Passivate();
		}

		odlet_nakladani++;
		q_nakladani.Insert(this);
		Passivate();

		//nastoupeni zakazniku
		Wait(2*MINUTA);
		
		//uzavri prepazky
		(new Uzavrit)->Activate();
		odlet_ovetrenych--;

		//signal dalsimu letaldu ze muze pro cestujici
		if( ! q_odlet_brany_letadla.Empty()) {
			q_odlet_brany_letadla.GetFirst()->Activate();
		}

		//uvolneni gatu pro nakladani
		odlet_nakladani--;
		//signal dalsimu letaldu
		if( ! q_odlet_nakladani.Empty()){
			q_odlet_nakladani.GetFirst()->Activate();
		}

		//rolovani na runway
		Wait(Uniform(5*MINUTA, 10*MINUTA));

		//odlet
		Seize(Runway);
		Wait (3*MINUTA);
		Release(Runway);

		odbaveni_letadla(Time-prilet);
		pocet_out_letadel++;
		
	}

};

//generátor letadel pro jeden den
class Gen_letadel : public Event {

	//maximalni pocet letu za den
	int max_letu_za_den;
	//vygenerovanych letu za den
	int vygen_letu;

	void Behavior() {

		//generovani letadel behem sezony
		if(je_sezona) {

			if(je_den && (vygen_letu < max_letu_za_den)) {
				(new Letadlo)->Activate();
				vygen_letu++;

				double dalsi;

				if(Random() < 0.20) {
					dalsi = Uniform(60*MINUTA, 80*MINUTA);

				} else if(Random() < 0.70){
					dalsi = Uniform(30*MINUTA, 60*MINUTA);
				} else {
					dalsi = Uniform(5*MINUTA, 30*MINUTA);
				}

				Activate(Time + dalsi);
			}

		//generovani mimo sezonu
		} else {

			if(je_den && (vygen_letu < max_letu_za_den)) {
				(new Letadlo)->Activate();
				vygen_letu++;

				Activate(Time + Uniform(HODINA, 6*HODINA));
			}
		}
	}

	public:
		Gen_letadel(){

			if(je_sezona) {
				max_letu_za_den = Uniform(10,16);
			} else {
				max_letu_za_den = Uniform(1,4);
			}
			vygen_letu = 0;

			Activate();
		}
};

//stridani dne a noci
class Gen_den : public Event {

	void Behavior() {

		if(je_den) {
			je_den = false;
			Activate(Time + (8*HODINA));

		//spusteni generatoru letadel pokud zacne den
		} else {
			je_den = true;
			(new Gen_letadel())->Activate();
			Activate(Time + (16*HODINA));
		}
	}
};

//stridani sezon
class Gen_sezona : public Event {

	void Behavior() {

		if(je_sezona) {
			je_sezona = false;
			Activate(Time + (6.5*MESIC));
		} else {
			je_sezona = true;
			Activate(Time + (5.5*MESIC));
		}
	}
};

//incializace a provedeni experimentu
int main() {

	RandomSeed(time(NULL));
#ifdef DEBUG
	SetOutput("stats.out");
#endif
	// inicializace experimentu
	Init(0,DOBA_SIMULACE);   

	// aktivace udalosti zmen dne, sezony a kontroly gatu
	(new Gen_sezona)->Activate();
	(new Gen_den)->Activate();	
	(new Nalozeni_cestujicich())->Activate();
	
	// simulace
	Run();

	//tisk statistik
	odbaveni_letadla.Output();
	cekani_bezp.Output();
	odbaveni_zakaznika.Output();


#ifdef DEBUG
	Print("Pocet prichozich zakazniku: %i\n", pocet_prichozich_zakazniku-1);
	Print("Pocet odchozich zakazniku:  %i\n", pocet_odchozich_zakazniku);

	Print("Pocet prichozich letadel: %i\n", pocet_in_letadel-1);
	Print("Pocet odchozich letadel: %i\n", pocet_out_letadel);
	Print("Velikost fronty na bezpecnostni prohlidku\n");
	bezp_fronta.Output();
	for (int i=0;i<ODLET_BRAN;i++) {
		if(q_odlet_brany[i].Length() > 0)
			Print("Fronta lidi cislo %i obsahuje %i\n",i,q_odlet_brany[i].Length());
	}
#endif

}