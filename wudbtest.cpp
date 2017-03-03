#include <iostream>
#include <sqlite3.h>
#include <cmath>
#include <ctime>
#include "DataTables.h"
using namespace std;

void addEZCutsFunctions( sqlite3 *db );
void ezwidth_func(sqlite3_context* context,int n,sqlite3_value** val);

double getTime();


int main(int argc, char* argv[]) {


    try {

	Database db("test.db");
    	
	HeaderRecord h;
	ParamRecord p;
	SimShowerRecord s;
	MuonRecord m;
	EZParamRecord e;

	h.setDatabaseHandle( db.getHandle() ); // must be called before anything works!
	p.setDatabaseHandle( db.getHandle() ); // must be called before anything works!
	s.setDatabaseHandle( db.getHandle() ); // must be called before anything works!
	m.setDatabaseHandle( db.getHandle() );
	e.setDatabaseHandle( db.getHandle() );

	addEZCutsFunctions(db.getHandle() );
	
	h.clearTable(); 
	p.clearTable();
	s.clearTable();
	m.clearTable();
	e.clearTable();

	h.sourcename="sgra*";
	h.nadc=490;
	h.writeToDatabase();

	for (int i=0; i<1000; i++) {
	    for (int j=0; j<4; j++) {
		p.event_number = i;
		p.telescope_id = j;
		p.size = i*4.0+j;
		p.writeToDatabase();
	    }
	}


	for (int i=0; i<100; i++) {
	    for (int j=0; j<4; j++) {
		s.event_number = m.event_number = i;
		s.telescope_id = m.telescope_id = j;
		s.primary_energy = i*0.1;
		m.muonness = i*101;

		s.writeToDatabase();
		m.writeToDatabase();
	    }
	}


	p.prepareToRead();

	while (p.readFromDatabase()) {
	    e.event_number = p.event_number;
	    e.telescope_id = p.telescope_id;
	    e.ezwidth = p.width*2+1;
	    e.ezlength = p.length*2+1;
	    e.ezsize = p.size*2+1;
	    e.writeToDatabase();
	}


	double start,end;
	int count;


	for (int k=0; k<10; k++) {
	    cout << "TEST: count(): "<< endl;
	    start = getTime();
	    count = p.count();
	    end= getTime();
	    cout << "\telapsed="<<end-start<<endl;
	    cout << "\tcount="<<count << endl;
	    
	    cout << "TEST: iterate: "<< endl;
	    count =0;
	    p.prepareToRead();
	    start = getTime();
	    while (p.readFromDatabase()) {
		count++;
	    }
	    end= getTime();
	    cout << "\telapsed="<<end-start<<endl;
	    cout << "\tcount="<<count << endl;
	}

	cout << "FINISHING"<<endl;



	s.finish();
	p.finish();
	h.finish();
	e.finish();

    }
    catch (runtime_error &e) {
	cerr << "RUNTIME ERROR: "<<e.what()<<endl;
    }
	
        
}

    
void addEZCutsFunctions( sqlite3 *db ) {
    
    sqlite3_create_function(
			    db, // database handle
			    "ezwidth", // function name
			    3, // number of arguments
			    SQLITE_UTF8, // text representation
			    NULL, // arbitrary pointer
			    ezwidth_func,
			    NULL, // xStep
			    NULL // xFinal
			    );


}


void ezwidth_func(sqlite3_context* context,int n,sqlite3_value** val) {

    const double WA = 0.003;
    const double WB = 0.04679;
    const double WC = 9.866;
    const double WD = 0.1832;
    const double WE = 0.01534;
    const double WF = 0.00248;
    const double WGAMMA = 0.949;
    const double WCOSPOW = 1.5;

    double wid = sqlite3_value_double( val[0] );
    double siz = sqlite3_value_double( val[1] );
    double zen = sqlite3_value_double( val[2] );

    //============================================================
    // Calculate ezwidth , ezlength
    //============================================================
    
    const double cos60 = cos(60.0*M_PI/180.0);
    double ezwidth2, ezlength2;
    double ezwidth, ezlength, wzenithfactor;
    double x,wshift2,lshift2,lterm1,wterm1;
    
    wzenithfactor = ( (pow(cos60, WGAMMA)/pow(cos60,WCOSPOW)) 
                            /pow( cos(zen), WGAMMA ));

    x=log(siz/0.4489); // need to divide out .4489 since fit
                              // values assume 490 camera with no
                              // corrections


    wshift2 = wid*wid - WA;
    wterm1 = (wshift2*wzenithfactor>1e-20)? 
        sqrt(wshift2*wzenithfactor): 1e-20;

    ezwidth2 = WA + pow( wterm1 - WB*(x-WC) - WE*pow(x-WC,2) 
                            - WF*pow(x-WC,3) ,2 );
    ezwidth = (ezwidth2>0.0)? sqrt(ezwidth2) : 0.0;

    cout << "DEBUG: ezwidth="<<ezwidth<<endl;

    sqlite3_result_double( context, ezwidth );

};




double getTime() {
    struct timeval tv;
    struct timezone tz;
    double time;

    gettimeofday( &tv,&tz);
    
    time = tv.tv_sec + (double)tv.tv_usec/1.0e6;
    
    return time;


}
