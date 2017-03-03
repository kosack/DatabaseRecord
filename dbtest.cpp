#include <iostream>
#include <sqlite3.h>
#include "DatabaseRecord.h"
using namespace std;

struct TestRecord : public DatabaseRecord {

    int i;
    double x;

    TestRecord() : DatabaseRecord() {
	addField( "i", i );
	addField( "x", x );
	setTableName("testtable");
    }

};

struct AnotherRecord : public DatabaseRecord {

    int number;
    std::string name;

    AnotherRecord() : DatabaseRecord() {
 	addField( "name", name );
	addField( "number", number );
	setTableName("names");
    }

};



int main(int argc, char* argv[]) {

    TestRecord rec;
    AnotherRecord a;

    rec.i=12;
    rec.x=3.14159265;

    cout << "TEST PRINTOUT OF A DatabaseRecord: "<<endl;
    cout << rec <<endl;
    
    sqlite3 *db;

    if (sqlite3_open( "test.db", &db )) {
	cout << "Couldn't open database"<<endl;
	exit(1);
    }

    try {
	
	rec.setDatabaseHandle( db ); // must be called before anything works!

	rec.clearTable(); // clear data that was there previously in
			  // table (it's appended otherwise)
	
	cout << "Writing a bunch of random events..."<<endl;
	for (int i=0; i<10000; i++){
	    rec.i=i;
	    rec.x=rand()/double(RAND_MAX);
	    rec.writeToDatabase();
	}
	cout << "Done."<<endl;
	
	cout << "READ BACK first 5 rows: "<<endl;
	rec.prepareToRead("1 limit 5"); // the 1 is a hack to allow a
					// proper "where" clause with
					// LIMIT in the SQL
					// statment. You can also
					// specify "x<0.5 limit 7" or something,
					// or leave it blank for no
					// "where", as shown later...
	
	while (rec.readFromDatabase()) {
	    cout << "i="<<rec.i<<" ";
	    cout << "x="<<rec.x<<endl;
	}


	// test the "count" function, which counts the number of rows
	// matching the criteria:

	cout << "TEST count: "<<rec.count()<<endl;
	char test[20];
	for (int i=0; i<10; i++) {
	    sprintf(test,"x<%f", i*0.1);
	    cout << "COUNT: x<"<<i*0.1<<" : "<<rec.count(test)<<endl;
	}

	
	// now print out some stuff for the other test stucture: note
	// values will be appended here, since I never call
	// clearTable()

	a.setDatabaseHandle( db );
	a.number = 42;
	a.name="Karl Kosack";
	a.writeToDatabase();

	cout << "READ BACK THE a STRUCTURE:" <<endl;

	a.prepareToRead();  // no "where", select all events

	while(a.readFromDatabase()){
	    cout << a << endl;
	}

	a.finish();
	rec.finish();


    }
    catch (runtime_error &e) {
	cout << "RUNTIME ERROR: "<<e.what()<<endl;
    }
	
    if (sqlite3_close(db))
	cout << "CLOSE: "<<sqlite3_errmsg(db)<<endl;
	
        
}

    
