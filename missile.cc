/* missile.c
 * jeff harlan
 * 03.08.00
 * 10.20.01
 *
 * missile command for X
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
//#include <libio.h>
#include <time.h>

char hello[]         =   " missile command ";
char end[]           =   " game over ";
char level_msg[][10] = { " level 0 ", " level 1 ", " level 2 ", " level 3 ",
                         " level 4 ", " level 5 ", " level 6 ", " level 7 ",
                         " level 8 ", " level 9 ", " level 10", " level 11" };

GC      mygc ;
XEvent  myevent ;
Display *mydisplay ;
Window  mywindow ;
unsigned long myforeground, mybackground ;

int     level_cycle_count = 0;
int     level[] = {  1,  5,  8, 12, 15, 
                    25, 30, 35, 40, 45,
                    50, 55, 60, 80, 99 };     // # missiles per level
int     my_level = 0;                         // current level
int     missiles_this_level = 0;
int     missile_timing[ 100 ];                // when to launch
int     base[] = { 100, 300, 500, 700 };      // x location of bases
int     live[] = {   1,   1,   1,   1, 0 };   // base status - 1 = ok
int     inc_loc = 1 ;                         // burst location increment 
int     inc_dia = 2 ;                         // burst diameter increment
int     game_over = 0 ;
int     debug = 0;
int     list_count = 0;                       // how many in the list?
int     last_list_count = 0;                  // how many last time?
int     del_count = 0;                        // how many deleted from the list?
int     loop;
int     lup;
int     tmp;
int     ndx=0;                                // missile_timing index

//
// screen_object base class for dynamic screen objects
//
class screen_object {  
    public:
      virtual void init( void ) {}
      virtual int cycle( void ) { return 0; }
      virtual void  hit( int, int, int, int ) {}
};

//
// linked_list class manages dynamic screen_objects
//
class linked_list {  
    public:
       class linked_list_element {
          public:
            screen_object *ptr;
          private:
            linked_list_element *next_ptr;
          friend class linked_list;
       };
    public: 
       int cycle_return;
       linked_list_element *first_ptr;
       linked_list( void ) { 
         first_ptr = NULL; 
         if( debug ) fprintf(stderr,"linked_list created\n");
       }
       void linked_list::add_list( screen_object * );
       int linked_list::cycle_list( void );
       void linked_list::destroy_list( void );
       void linked_list::hit_list( int x1, int y1, int x2, int y2 );
};

//
// add new node to list - node contains pointer to screen_object
//
void linked_list::add_list( screen_object *obj ) {  

    if( debug ) fprintf(stderr, "linked_list::add_list called\n" );
    linked_list_element *new_ptr;

    new_ptr = new linked_list_element;
    ( *new_ptr ).ptr = obj;
    ( *new_ptr ).next_ptr = first_ptr;
    first_ptr = new_ptr;
};

//
// iterate through list calling cycle() method
//      let each screen object update itself in
//      a small time slice
//
int linked_list::cycle_list( void ) {  

    if( debug ) fprintf(stderr, "linked_list::cycle_list called\n" );
    linked_list_element *index_ptr;

    index_ptr = first_ptr;
    list_count = 0;
    cycle_return = 0;
    while( index_ptr ) {
      list_count++;
      if( index_ptr->ptr->cycle() ) cycle_return = 1 ;  // works with initiator
      index_ptr = ( *index_ptr ).next_ptr;;
    }
    if( debug ) {
       if( list_count != last_list_count ) {
          if(debug) fprintf(stderr, "%d ", list_count );
          last_list_count = list_count;
       }
    }
    return cycle_return;
};

//
// iterate through list calling hit() method  
//       called when anti-missile bursts are at
//       their max diameter to have missiles
//       detect if they were hit
//
void linked_list::hit_list( int x1, int y1, int x2, int y2 ) {  

    if( debug ) fprintf(stderr, "linked_list::hit_list called\n" );
    linked_list_element *hit_ptr;

    hit_ptr = first_ptr;
    while( hit_ptr ) {
      hit_ptr->ptr->hit( x1, y1, x2, y2 );           
      hit_ptr = ( *hit_ptr ).next_ptr;;
    }
};

//
// iterate through list deleting both screen objects 
//      and list nodes
//
void linked_list::destroy_list( void ) {  

    if( debug ) fprintf(stderr, "linked_list::destroy_list called\n" );
    linked_list_element *del_ptr;

    del_ptr = first_ptr;               // initialize delete pointer for while
    del_count = 0;                     // initialize deleted node count
    while( del_ptr ) {                 // keep going until NULL pointer
       del_count++;                    // count deleted nodes
       del_ptr = first_ptr;            // re-initialize delete pointer
       delete del_ptr->ptr;            // delete screen object
       del_ptr = del_ptr->next_ptr;    // move index to next node before del
       delete first_ptr;               // delete linked list node
       first_ptr = del_ptr;            // move first_ptr to next undeleted node
    }
    if( debug ) fprintf(stderr, "%d :destroy_list objects deleted\n", del_count );
};

linked_list list;

//
// anti-missile burst class
//       user clicks create new bursts 
//
class burst: public screen_object {      // anti-missile burst class
    private:
       int x, y, xkeep, ykeep, diameter, loop;
    public:
       burst( int x_in, int y_in ) {     
           xkeep = x_in;
           ykeep = y_in;
           x = xkeep;
           y = ykeep;
           diameter = 0;
           loop = 0;
       }
       int cycle( void ) {   // method lets burst object make single update
           if( debug ) fprintf(stderr, "burst::cycle called\n" );
           loop++;
           if( loop < 20 ) {          // draw burst
              XSetForeground( mydisplay, mygc, myforeground ) ; 
           } else {                   // erase burst
              XSetForeground( mydisplay, mygc, mybackground ) ; 
              if( loop >= 40 ) {      // burst has erased itself - done
                 XSetForeground( mydisplay, mygc, myforeground ) ;  
                 return 0;
              } else {
                 if( loop == 20 ) {   // max burst diameter - time to erase
                    // detect if any missiles hit
                    list.hit_list( xkeep-20, ykeep-20, xkeep+20, ykeep+20 );
                    diameter = 0;   
                    x = xkeep;
                    y = ykeep;
                 }                 
              }
           }

           // draw or erase burst
           XFillArc( myevent.xexpose.display, myevent.xexpose.window, mygc,
              x, y, diameter, diameter, 0, 360*64 );
    //       XDrawArc( myevent.xexpose.display, myevent.xexpose.window, mygc,
    //          x-=inc_loc, y-=inc_loc, diameter, diameter, 0, 360*64 );
           diameter+=inc_dia;
           x-=inc_loc;
           y-=inc_loc;

           // reset color - be nice to other drawing routines
           XSetForeground( mydisplay, mygc, myforeground ) ;  
           return 1;
       }
};

//
// incoming missile class
//      randomly generated missiles which target live bases
//
class missile: public screen_object {     // incoming missile class
    private:
       int x, y, alive, nx;
       float dx;                // non-integer to allow small x increments
       int rand_base;
    public:
       missile( void ) {
           if( debug ) fprintf(stderr, "missile created\n");
           x = 1+(int) (800.0*rand()/(RAND_MAX+1.0));    // start missile
           nx = x;                                       // new horizontal value
           rand_base = 4;                                // dummy destroyed 
           while( ! live[ rand_base ] && ! game_over) {  // target live bases
              rand_base = (int) (4.0*rand()/(RAND_MAX+1.0)); // base to attack
           }
           if( debug ) fprintf(stderr, "%d : rand_base\n", rand_base );
           dx = (float)( x - base[rand_base]+7 ) / 599.0;
           if( debug ) fprintf(stderr, "%f : dx \n", dx );
           y = 0;           // initial vertical value
           alive = 1;       // missile has not been hit with burst 
       }
       int cycle( void ) {
if(debug) fprintf(stderr,"missile::cycle()\n");
           if( alive ) {        // are we already dead?
if(debug) fprintf(stderr,"missile::cycle if( alive )\n");
              nx = (int)((float)x - (float) y  * dx + 13.0);
if(debug) fprintf(stderr,"missile::cycle if( alive ) x: %d y: %d dx: %f\n", x, y, dx);
              if( y >= 599 ) {           // bottom reached yet?
                 // end missile
                 live[ rand_base ] = 0;      // base destroyed 
                 list.add_list(              // base explosion
                     new burst( nx , y ));
                 y++;
                 alive = 0;             // missile was hit
                 return 0;
              } else {
if(debug) { 
  fprintf(stderr,"XDrawPoint: x: %d nx: %d y %d\n", x, nx, y);
  if( myevent.xexpose.display )  fprintf(stderr,"myevent.xexpose.display\n");
  if( myevent.xexpose.window )   fprintf(stderr,"myevent.xexpose.window\n");
  if( mygc )                     fprintf(stderr,"mygc\n");
}
                 XDrawPoint( myevent.xexpose.display, myevent.xexpose.window,
                        mygc, nx , y );  

if(debug) fprintf(stderr,"(skipped) after XDrawPoint\n");
                 y++;                  // missile travel increment
              }
              return 1;
           }
           return 0;
       }
       void hit( int x1, int y1, int x2, int y2 ) {   // have we been hit?
           if( nx >= x1 && nx <= x2 && y >= y1 && y <= y2 ) alive = 0;
       }
};

void draw_title( void ) {
    XDrawImageString( 
        myevent.xexpose.display, myevent.xexpose.window, mygc,
        20, 30, hello, strlen( hello ) ) ;
}

void draw_base( int x_loc ) {      // draw individual base
    XFillRectangle( myevent.xexpose.display, myevent.xexpose.window, mygc, 
                     x_loc, 585, 15, 15);
    XFillRectangle( myevent.xexpose.display, myevent.xexpose.window, mygc, 
                     x_loc+6, 582, 3, 3);
}

void draw_bases( void ) {
   for( loop=0; loop<4; loop++ ) {    // draw all live bases
       if( live[ loop ] ) draw_base( base[loop] );
   }
}

void clear_screen( void ) {       // set the screen to the background color
   XSetForeground( mydisplay, mygc, mybackground ) ; 
   XFillRectangle( myevent.xexpose.display, myevent.xexpose.window,
      mygc, 0, 0, 800, 600);
   XSetForeground( mydisplay, mygc, myforeground ) ; 
}

void next_level( void ) {
   if( debug ) fprintf(stderr, "next_level  my_level: %d\n", my_level);
   my_level++;
   level_cycle_count = 0;
   missiles_this_level = 0;
   XSetForeground( mydisplay, mygc, myforeground ) ; 
   clear_screen(); 
   draw_bases();
   draw_title();
   XDrawImageString(
      myevent.xexpose.display, myevent.xexpose.window, mygc,
      350, 280, level_msg[ my_level ], 
      strlen( level_msg[ my_level ] ) ) ;
   XFlush( mydisplay );
   sleep( 2 );
   for( loop=0; loop<=level[ my_level ]; loop++ ) {
      missile_timing[ loop ] = 1+(int) (800.0*rand()/(RAND_MAX+1.0));   
   }
   for( loop=0; loop<=level[ my_level ] ; loop++ ) {
      for( lup=loop+1; lup<=level[ my_level ] ; lup++ ) {
         if( missile_timing[ loop ] > missile_timing[ lup ] ) {
            tmp = missile_timing[lup];
            missile_timing[lup] = missile_timing[loop];
            missile_timing[loop] = tmp;
         }
      }
   }
   clear_screen();
   draw_bases();
}

int main(int argc, char **argv) {

 srand( time( NULL ) );          // init random number generator

  KeySym  mykey ;
  XSizeHints myhint ;
  Font    font ;
  int     wx, wy;
  unsigned int wwidth, wheight, wdepth, wborder ;
  int     myscreen ;
  int     i ;
  char    text[10] ;
  int     loop ;
  int     sleep_time = 12000 ;
  int     done ;
  int     not_paused = 1 ;
  int     first_main_loop = 1;
  
 
if(debug) fprintf(stderr,"begin main()\n");
  mydisplay = XOpenDisplay("") ;
  myscreen = DefaultScreen(mydisplay) ;
  mybackground = BlackPixel(mydisplay, myscreen) ;
  myforeground = WhitePixel(mydisplay, myscreen) ;
  myhint.x = 40 ;  myhint.y = 100 ;
  myhint.width = 800 ;  myhint.height = 600 ;
  myhint.flags = PPosition | PSize ;

  mywindow = XCreateSimpleWindow(mydisplay,
      DefaultRootWindow(mydisplay),
      myhint.x, myhint.y, myhint.width, myhint.height,
      1, myforeground, mybackground) ;

  XSetStandardProperties(mydisplay, mywindow, hello, hello,
      None, argv, argc, &myhint) ;
  mygc = XCreateGC(mydisplay, mywindow, 0, 0);
  XSetBackground(mydisplay, mygc, mybackground) ;
  XSetForeground(mydisplay, mygc, myforeground) ;

  XSelectInput(mydisplay, mywindow,
      ButtonPressMask | KeyPressMask | ExposureMask) ;
 
  XMapRaised(mydisplay, mywindow) ;

  font = XLoadFont ( mydisplay, "12x24" );
  XSetFont ( mydisplay, mygc, font );
  XGetGeometry ( mydisplay, mywindow, &mywindow, &wx, &wy,
                 &wwidth, &wheight, &wborder, &wdepth );
  XDefineCursor ( mydisplay, mywindow, None ) ;

  done = 0 ;
if(debug) fprintf(stderr,"before main while()\n");
  while(done==0) {
if(debug) fprintf(stderr,"top of main while()\n");
     XFlush(mydisplay) ;
     if( XEventsQueued(mydisplay,QueuedAfterFlush) || first_main_loop ) {
      XNextEvent(mydisplay, &myevent) ;
    
      if( first_main_loop ) {
        first_main_loop = 0;
        next_level();   
      }
if(debug) fprintf(stderr,"myevent.type: %d\n", myevent.type);
      switch(myevent.type) {
        case Expose:
        if(myevent.xexpose.count==0)
           draw_bases();
        break ;
   
        case MappingNotify:
           XRefreshKeyboardMapping((XMappingEvent *)&myevent) ;
        break ;
  
        case ButtonPress:
           list.add_list(
               new burst( myevent.xbutton.x, myevent.xbutton.y ) );
        break ;
  
        case KeyPress:
           i=XLookupString((XKeyEvent *)&myevent, text, 10, &mykey, 0) ;
           if(i==1 && text[0]=='n') { 
              for( loop=0; loop<3; loop++ ) {
                 list.add_list( new missile );
              }
           }
           if(i==1 && text[0]=='c') {
               clear_screen();
               draw_bases();
           }
           if(i==1 && text[0]=='d') list.destroy_list();  // one guess
           if(i==1 && text[0]=='q') done=1 ;
           if(i==1 && text[0]=='p') {
               if (not_paused) {
                  not_paused = 0;
               }else{
                  not_paused = 1;
               }
           };
        break ;
     } /* switch myevent.type */
    } /* if XeventsQueued */
if(debug) fprintf(stderr, "after if XeventsQueued\n");
    if (not_paused) { 
       usleep(sleep_time) ; 

       if( missiles_this_level < level[ my_level ] ) {   // all missiles?

          if( missile_timing[ ndx ] <= level_cycle_count ) {  // sorted timing
             if( ! game_over ) list.add_list( new missile );
             missiles_this_level++;                           // another missile
             ndx++;                                           // sorted index
          } 
          list.cycle_list();    // cycle objects
       } else {

          if( ! list.cycle_list() && ! game_over ) {  // level complete?
             for( loop=0; loop<100; loop++ ) {
                list.cycle_list();
                XFlush( mydisplay );
                usleep( sleep_time );
             }
             next_level();      // go to next level
             ndx=0;
          }
       }
       level_cycle_count++;
    } /* if not_paused */
    if( live[0]==0 && live[1]==0 && live[2]==0 && live[3]==0 ) {
       game_over=1;
    }
    if( game_over ) {     // wait for base to explode
       XDrawImageString(
         myevent.xexpose.display, myevent.xexpose.window, mygc,
         350, 280, end, strlen( end ) ) ;
       done = 1;
       for( loop=0; loop<600; loop++ ) {
          list.cycle_list();
          XFlush( mydisplay );
          usleep( sleep_time );
       }
       sleep( 1 );
    }
   } /* while done==0 */

   
   XFreeGC(mydisplay, mygc) ;
   XDestroyWindow(mydisplay, mywindow) ;
   XCloseDisplay(mydisplay) ;
   exit(0) ;
} /* main */
