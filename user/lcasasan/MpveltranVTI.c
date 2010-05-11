/* Slope-based tau-p velocity transform for VTI media by Stripping Equations. */
/*
  Copyright (C) 2010 Politecnico di Milano
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <math.h>
#include <float.h>
#include <rsf.h>

int main (int argc, char* argv[])
{
    bool inter;
    int it,ix,ip,iv, nt,nx, np, nv, ntv, nw, nvh,ntvh ;
    float dt, t0, p, p0, t, dp, v0, dv, vh0, dvh;
    float N, D, Nt, Dt;
    float *R=NULL, *Rt=NULL, *Q=NULL, *Qt=NULL, **coord1=NULL, **coord2=NULL, **coord3=NULL ,*ord=NULL;
    float *vN=NULL, *vN2=NULL,*vH=NULL, *vH2=NULL, *e=NULL, *e2=NULL;
    sf_file cmp=NULL, velN=NULL, velH=NULL, dip=NULL, dipt=NULL, curv=NULL, curvt=NULL, eta=NULL;
    /*debug objects*/
    //sf_file  temp=NULL; 
    //float *tempvec=NULL;
    /* eta axis fixed inside the code */
    float e0 , de, nte;
    int ne , ie;
	
    sf_init (argc,argv);
    cmp = sf_input("in");
	velN = sf_output("out");

    
	if (NULL != sf_getstring("dip")) dip = sf_input("dip"); /*slope field*/
	else dip = NULL ;
	if (NULL != sf_getstring("curv")) curv = sf_input("curv"); /*curvature field*/
	else curv = NULL ;
	
	if (NULL != sf_getstring("velH")) velH = sf_output("velH"); /*horizontal velocity*/
	else velH = NULL ;
	if (NULL != sf_getstring("eta")) eta = sf_output("eta"); /*eta*/
	else eta = NULL ;
	
    
    //temp = sf_output("temp");
    
    if (SF_FLOAT != sf_gettype(cmp)) sf_error("Need float input");
    if (!sf_histint(cmp,"n1",&nt)) sf_error("No n1= in input");
    if (!sf_histfloat(cmp,"d1",&dt)) sf_error("No d1= in input");
    if (!sf_histfloat(cmp,"o1",&t0)) sf_error("No o1= in input");
    
    if (!sf_histint(cmp,"n2",&np)) sf_error("No n2= in input");
    if (!sf_histfloat(cmp,"d2",&dp)) sf_error("No d2= in input");
    if (!sf_histfloat(cmp,"o2",&p0)) sf_error("No o2= in input");
    p0 /= dp; /* slope axis is normalized: no need for slope normalization dp=1*/
    
    
    /*CDPtype=1;
    if (sf_histfloat(cmp,"d3",&dy)) {
    CDPtype=0.5+0.5*dp/dy;
    if (1 != CDPtype) sf_histint(cmp,"CDPtype",&CDPtype);
    }
    if (CDPtype < 1) CDPtype=1;
    sf_warning("CDPtype=%d",CDPtype);*/
    
    //first velocity... VNMO
    if (!sf_getint("nv",&nv)) sf_error("Need nv=");     /* number of velocities */
    if (!sf_getfloat("v0",&v0)) sf_error("Need v0=");     /* velocity origin */
    if (!sf_getfloat("dv",&dv)) sf_error("Need dv=");     /* velocity sampling */
    //second velocity... VH
    if (!sf_getint("nvh",&nvh)) nvh=nv;     /* number of HOR velocities  */
    if (!sf_getfloat("vh0",&vh0)) vh0=v0;     /* HOR velocity origin */
    if (!sf_getfloat("dvh",&dvh)) dvh=dv;     /* HOR velocity sampling */ 
    //anellipticity
    if (!sf_getint("ne",&ne))   ne = 101;     /* number of etas */
    if (!sf_getfloat("e0",&e0)) e0 = -0.5;    /* eta origin */
    if (!sf_getfloat("de",&de)) de=0.01;      /* eta sampling */ 
    
    
    /* adding the second dimension to the ouput files*/
    sf_putint(velN,"n2",nv);
    sf_putfloat(velN,"o2",v0);
    sf_putfloat(velN,"d2",dv);
    
    sf_putint(velH,"n2",nvh);
    sf_putfloat(velH,"o2",vh0);
    sf_putfloat(velH,"d2",dvh);

    sf_putint(eta,"n2",ne);
    sf_putfloat(eta,"o2",e0);
    sf_putfloat(eta,"d2",de);
    
    /* reading the number of cmp in the data */
    nx = sf_leftsize(cmp,2);
    
    if (!sf_getint("nw",&nw)) nw=4;
    /* interpolator size (2,3,4,6,8) */
    
    ntv = nt*nv;
    ntvh= nt*nvh;
    
    R = sf_floatalloc(nt);
    Q = sf_floatalloc(nt);
    
    coord1 = sf_floatalloc2(2,nt);
    coord2 = sf_floatalloc2(2,nt);
    coord3 = sf_floatalloc2(2,nt);    

    ord = sf_floatalloc(nt);
    vN  = sf_floatalloc(ntv);
    vN2 = sf_floatalloc(ntv);
    vH  = sf_floatalloc(ntvh);
    vH2 = sf_floatalloc(ntvh);
    
    nte = nt*ne;
    e  = sf_floatalloc(nte);
    e2 = sf_floatalloc(nte);
//    tempvec = sf_floatalloc(nt);



    if (!sf_getbool("interval",&inter)) inter=false;
    /* if y, compute interval velocity */
    
    if (inter) {

        dipt = sf_input("dipt");
        curvt =sf_input("curvt");
        Rt = sf_floatalloc(nt);
        Qt = sf_floatalloc(nt);


    } else {
        dipt = NULL;
        curvt =NULL;
        Rt = NULL;
        Qt = NULL;
    }
    sf_warning("\nHere I am %f\n",p0);
    for (ix = 0; ix < nx; ix++) { /* CMP loop*/
        
        for (iv=0; iv < ntv; iv++) {
            vN[iv]=0.;
        }
		for (iv=0; iv < ntvh; iv++) {
            vH[iv]=0.;
        }
        for (ie=0; ie< nte; ie++) {
	    e[ie]=0.;
        }
        for (ip = 0; ip < np; ip++) { /* slope p loop*/
        /*h = h0 + (ih+0.5)*dh + (dh/CDPtype)*(ix%CDPtype);*/
            p=p0+ip;
	    //p=p0+ip*dp;
            /*sf_warning("\np0 %f",dp);*/

            sf_floatread (ord, nt, cmp);	    
            sf_floatread (  R, nt, dip);
            sf_floatread (  Q, nt, curv);
           


            if (inter) {
                sf_floatread (Rt, nt, dipt);
                sf_floatread (Qt, nt, curvt);
            }

            for (it=0; it < nt; it++) { /* time tau loop*/
                t = t0 + it*dt;
                
                N=(3*t*R[it]*(dt)+t*p*Q[it]*(dt)-3*R[it]*R[it]*(dt*dt)*p);
                D=(3*t*R[it]*(dt)+t*p*Q[it]*(dt)+1*R[it]*R[it]*(dt*dt)*p);
              	//N=3*t*R[it]*(dt/dp)+t*p*Q[it]*(dt/dp/dp)-3*R[it]*R[it]*(dt*dt/dp/dp)*p;
                //D=3*t*R[it]*(dt/dp)+t*p*Q[it]*(dt/dp/dp)+1*R[it]*R[it]*(dt*dt/dp/dp)*p;
                
        /*f = t - p[it]*h*dt/dh;*/
                
                if (N*D < 0.) {
                    coord1[it][0]=t0-10.*dt;    // zero offset traveltime...
                    coord1[it][1]=0.;
                    
                    coord2[it][0]=t0-10.*dt;    // zero offset traveltime...
                    coord2[it][1]=0.;

	            coord3[it][0]=t0-10.*dt;    // zero offset traveltime...
                    coord3[it][1]=0.;
                    
                } else {
                    coord1[it][0] = t*sqrtf( (N*D)/(D*(D+FLT_EPSILON) ) );
                    coord2[it][0] = t*sqrtf( (N*D)/(D*(D+FLT_EPSILON) ) );
                    coord3[it][0] = t*sqrtf( (N*D)/(D*(D+FLT_EPSILON) ) );

                    if (inter) {			/* INTERVAL VELOCITIES with taup stripping*/
            
                      /*Nt=3*Rt[it]*(1/dp)+p*Qt[it]*(1/dp/dp)-3*Rt[it]*Rt[it]*(1/dp/dp)*p;
                        Dt=3*Rt[it]*(1/dp)+p*Qt[it]*(1/dp/dp)+1*Rt[it]*Rt[it]*(1/dp/dp)*p;
			coord1[it][1] =
                        sqrtf(fabsf( 
                        (-1/p)*(16*Rt[it]*Rt[it]*Rt[it]*(1/dp/dp/dp) )
                        /(Nt*(Dt+FLT_EPSILON) )
                        ) );
			coord2[it][1] =
                        sqrtf(fabsf( (1/(p*p) )*
                        ( (Nt-4*Rt[it]/dp) /(Nt) )
                        ) );	*/                        

			Nt=(3*Rt[it]+p*Qt[it]-3*Rt[it]*Rt[it]*p);
                        Dt=(3*Rt[it]+p*Qt[it]+1*Rt[it]*Rt[it]*p);
                        coord1[it][1] =
                        sqrtf(fabsf( 
                        (-1/p)*(16*Rt[it]*Rt[it]*Rt[it]*(1/dp/dp) )
                        /(Nt*(Dt+FLT_EPSILON) )
                        ) );
                        
			//tempvec[it]= coord1[it][1];
                        
			coord2[it][1] =
                        sqrtf(fabsf( (1/(p*p*dp*dp) )*
                        ( (Nt-4*Rt[it]) /(Nt) )
                        ) );


                        
                        
                    } else {				/* EFFECTIVE VELOCITIES */
                        
                      /*coord1[it][1] = sqrtf(fabsf( 
			(-1/p)*(16*t*R[it]*R[it]*R[it]*(dt*dt*dt/dp/dp/dp) )
                        /(N*D)
                        ) );

                        coord2[it][1] = sqrtf(fabsf( (1/(p*p)) * (
                        (N-4*t*R[it]*(dt/dp) ) /(N) )
                        ) );*/

			coord1[it][1] = sqrtf(fabsf( 
			(-1/p)*(16*t*R[it]*R[it]*R[it]*(dt*dt*dt/dp/dp) )
                        /(N*D)
                        ) );
			
			//tempvec[it]= coord1[it][1];		

                        coord2[it][1] = sqrtf(fabsf( (1/(p*p*dp*dp)) * (
                        (N-4*t*R[it]*(dt) ) /(N) )
                        ) );
                        

                    }
			coord3[it][1] = .5* ( (coord2[it][1]*coord2[it][1] / coord1[it][1]/coord1[it][1]) -1 );			
                        //sf_warning("\nVn %f Vh %f",coord1[it][1],coord2[it][2]);
                }
            } /* END tau t loop */

            sf_int2_init (coord1, t0,v0, dt,dv, nt,nv, sf_spline_int, nw, nt);
            sf_int2_lop (true,true,ntv,nt,vN,ord);

	    
            sf_int2_init (coord2, t0,vh0, dt,dvh, nt,nvh, sf_spline_int, nw, nt);
            sf_int2_lop (true,true,ntvh,nt,vH,ord);
            

	        sf_int2_init (coord3, t0,e0, dt,de, nt,ne, sf_spline_int, nw, nt);
            sf_int2_lop (true,true,nte,nt,e,ord);

	    //sf_floatwrite (tempvec,nt,temp);
        } /* END slope p loop */
        
    /* from spline coefficients to model */
        if (nw > 2) { /* loop spline */
          for (iv=0; iv < nv; iv++) {
                sf_spline_post (nw, iv*nt, 1, nt, vN, vN2);
           }
            for (iv=0; iv < nvh; iv++) {
                sf_spline_post (nw, iv*nt, 1, nt, vH, vH2);
           }

          for (ie=0; ie < ne; ie++) {
				sf_spline_post (nw, ie*nt, 1, nt, e, e2);
          }
     
            for (it=0; it < nt; it++) {
               sf_spline_post (nw, it, nt, nv, vN2, vN);
               sf_spline_post (nw, it, nt, nv, vH2, vH);
	       sf_spline_post (nw, it, nt, ne, e2, e);
            }
        } /* END loop spline */
        
        sf_floatwrite (vN,ntv,velN);
        sf_floatwrite (vH,ntvh,velH);
        sf_floatwrite (e,nte,eta);
    } /* END CMP loop */
    sf_close();
    exit (0);
} /* END of main */
