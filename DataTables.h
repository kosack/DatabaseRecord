// Karl Kosack, 2005
#ifndef DATATABLES_H
#define DATATABLES_H

#include "DatabaseRecord.h"


// some simple helper structs:

/**
 * A 2-d coordinate.
 */
struct Coordinate_t {
    double x;  //!< X coordinate
    double y; //!< Y coordinate
};


// =========================================================================
// Definitions for all WUParam data tables that can be written/read
// using the DatabaseRecord interface.  These should all be subclasses
// of DatabaseRecord, and have all their fields mapped to names in
// their constructors.  They will then inherit the ability to be
// read/written from a database file and be printed out using
// cout. See DatabaseRecord.h for more info.
//

/**
 * Data which is written out by the parameterizer
 */
struct ParamRecord : public DatabaseRecord {

    int    event_number;        //!< Event number for debugging
    int    telescope_id;        //!< telescope id number
    double osctime;             //!< oscillator time
    double gpstime;             //!< calibrated gps time
    double livetime;            //!< livetime of run
    short  invalid;             //!< was this event properly parameterized?
    Coordinate_t centroid;    	//!< Center position of shower 
    Coordinate_t point_of_origin_a;	//!< First point of origin (x,y)
    Coordinate_t point_of_origin_b;	//!< second point of origin (x,y)
    double length;		//!< Length of ellipse
    double width;		//!< width of ellipse
    double size;                //!< total signal
    double miss;		//!< miss parameter
    double distance; 		//!< distance to centroid
    double azwidth;             //!< used by certain analysis techniques
    double alpha;		//!< Alpha angle (-Pi/2..Pi/2)
    double length_over_size;    //!< length over size
    double psi;			//!< Angle betw major axis and x-axis (-PI..PI)
    double phi;                 //!< Angle that centroid makes with x-axis
    double max[3];		//!< The three largest adc values
    int    index_of_max[3]; 	//!< The tube indices of the max[3] values
    double frac[3]; 		//!< fraction of maximum digital counts
    int    pixels_in_picture;   //!< number of tubes in the picture
    double asymmetry; 		//!< measure of shower skew
    double zenith;              //!< zenith angle of event

    // not sure if these should be separate table:
    double energy_estimate;     //!< estimated energy - computed at time of cut

    ParamRecord() : DatabaseRecord() {
	setTableName("paramdata");
	addField( "event_number", event_number );
	addField( "telescope_id", telescope_id );
	addField( "osctime",   osctime );
	addField( "gpstime",   gpstime );
	addField( "livetime",  livetime );
	addField( "centroid_x", centroid.x );
	addField( "centroid_y", centroid.y );
	addField( "poo_a_x", point_of_origin_a.x );
	addField( "poo_a_y", point_of_origin_a.y );
	addField( "poo_b_x", point_of_origin_b.x );
	addField( "poo_b_y", point_of_origin_b.y );
	addField( "length", length );
	addField( "width", width );
	addField( "size", size );
	addField( "miss", miss );
	addField( "distance", distance );
	addField( "azwidth", azwidth );
	addField( "lensize", length_over_size );
	addField( "psi", psi );
	addField( "phi", phi );
	addField( "max1", max[0] );
	addField( "max2", max[1] );
	addField( "max3", max[2] );
	addField( "imax1", max[0] );
	addField( "imax2", max[1] );
	addField( "imax3", max[2] );
	addField( "frac1", frac[0] );
	addField( "frac2", frac[1] );
	addField( "frac3", frac[2] );
	addField( "pix_in_picture", pixels_in_picture );
	addField( "asymmetry", asymmetry );
	addField( "zenith", zenith );
	addField( "e_est", energy_estimate );
	zero();
    }

};

struct EZParamRecord : public DatabaseRecord {

    int event_number;
    int telescope_id;
    double ezlength;
    double ezwidth;
    double ezsize;


    EZParamRecord() : DatabaseRecord() {
	setTableName("ezparams");
	addField( "event_number", event_number );
	addField( "telescope_id", telescope_id );
	addField( "ezlength", ezlength );
	addField( "ezwidth", ezwidth );
	addField( "ezsize", ezsize );
    }

};


/**
 * Table of simulation parameters for each event. The 'event_number'
 * field should match the event number for the parametrized data so
 * this table can be cross-referenced.
 */
struct SimShowerRecord : public DatabaseRecord {
    int event_number;
    int telescope_id;
    int primary_type;
    double primary_energy;
    Coordinate_t impact_parameter;
    Coordinate_t direction_cos;

    SimShowerRecord() : DatabaseRecord() {
	setTableName("simdata");
	addField( "event_number",event_number );
	addField( "telescope_id",telescope_id );
	addField( "primary_type",primary_type );
	addField( "primary_energy", primary_energy );
	addField( "impact_param_x", impact_parameter.x );
	addField( "impact_param_y", impact_parameter.y );
	addField( "dir_cos_x",direction_cos.x );
	addField( "dir_cos_y",direction_cos.y );
	zero();
    }

};



/**
 * Header info for parameterized data
 */
struct HeaderRecord : public DatabaseRecord {

    int nadc;
    double ra;
    double dec;
    double starttime;
    double endtime;
    double average_elevation;
    int windowsize;
    std::string sourcename;
    std::string runid;
    std::string pairid;

    HeaderRecord() : DatabaseRecord() {
	setTableName("paramheader");
	addField( "nadc", nadc );
	addField( "ra", ra );
	addField( "dec", dec );
	addField( "starttime", starttime );
	addField( "endtime", endtime );
	addField( "average_elevation", average_elevation );
	addField( "windowsize", windowsize );
	addField( "sourcename", sourcename );
	addField( "runid", runid );
	addField( "pairid", pairid );
	zero();
    };


};


/**
 * Describes the muon-like characteristics of an image
 */
struct MuonRecord : public DatabaseRecord {

    static const int MAX_PLOT_CENTERS = 10000;

    int event_number;
    int telescope_id;
    double  radius ;   		//!< Average radius 
    Coordinate_t ringcenter ;	//!< Average position
    double  arcstrength ;	//!< fraction of centers in peak bin
    double  gain ;		//!< Total picture light per all pixels in arc
    double  mugain ;		//!< Light in annulus corrected for spillover
    double  ringfrac;         	//!< fraction of ring falling on camera
    double  soal ;		//!< Signal over arclength
    double  muskew ;		//!< centroid->center length over ring radius
    double  arclen ;		//!< Length of the muon arc
    double  philo;              //!< starting angle of arc
    double  phihi;              //!< ending angle of arc
    double  phimid;             //!< midpoint angle
    double muonness; 		//!< measure of muon-like characteristics
    double smoothness;          //!< measure of angular smoothness of ring
    double smoothness_var;      //!< angular smoothness variance of ring
    double xcs;
    double ycs;
    double rspread;

    // not stored in database:
    int	nplotpoints ;		//!< number of ring positions
    Coordinate_t plot[MAX_PLOT_CENTERS]; //!< Array of ring center positions 


    MuonRecord () : DatabaseRecord () {
	setTableName("muondata");
	addField( "event_number", event_number );
	addField( "telescope_id", telescope_id );
	addField( "radius", radius);
	addField( "ringcenter_x", ringcenter.x);
	addField( "ringcenter_y", ringcenter.y);
	addField( "arcstrength", arcstrength);
	addField( "gain", gain);
	addField( "mugain", mugain);
	addField( "ringfrac", ringfrac);
	addField( "soal", soal);
	addField( "muskew", muskew);
	addField( "arclen", arclen);
	addField( "philo", philo);
	addField( "phihi", phihi);
	addField( "phimid", phimid);
	addField( "muonness", muonness);
	addField( "smoothness", smoothness);
	addField( "smoothness_var", smoothness_var);
	addField( "xcs", xcs);
	addField( "ycs", ycs);
	addField( "rspread", rspread);
	zero();
    }
};

#endif
