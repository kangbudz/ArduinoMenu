/*
dynamic menu tests (experimental)

this is a somehow dynamic menu, its ok to use it on non AVR's
or with USE_RAM defined

the dynamic support is somehow limited
we do not support adding or removing options
however changing the list of options should be allowed

or replacing options

the user is responsible for all option allocation
deleting them if created with new, etc...

this can turn out to be a mess

objects construction is not really tested beyond this example
please let me know if you have any interest on this kind of things

this is nice to support dynamic options like a list of detected wifi networks
or a list of files+folders from a folder/file system
*/

#include <menu.h>
#include <AnsiStream.h>
#include <menuIO/ansiSerialOut.h>
#include <menuIO/serialIn.h>
#include <Streaming.h>

using namespace Menu;

#define LED LED_BUILTIN

#ifndef USING_RAM
#error "This menu demo does not work on flash memory versions (MENU_USEPGM)"
#error "Library must be compiled with MENU_USERAM defined (default for non AVR's)"
#error "ex: passing -DMENU_USERAM to the compiler"
#endif

// define menu colors --------------------------------------------------------
//each color is in the format:
//  {{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
const colorDef<uint8_t> colors[] MEMMODE={
  {{BLUE,WHITE}  ,{BLUE,WHITE,WHITE}},//bgColor
  {{BLACK,BLACK} ,{WHITE,BLUE,BLUE}},//fgColor
  {{BLACK,BLACK} ,{YELLOW,YELLOW,RED}},//valColor
  {{BLACK,BLACK} ,{WHITE,BLUE,YELLOW}},//unitColor
  {{BLACK,BLACK} ,{BLACK,BLUE,RED}},//cursorColor
  {{BLACK,BLACK}  ,{BLUE,RED,BLUE}},//titleColor
};

//choose field and options -------------------------------------
int duration=0;//target var
prompt* durData[]={
  new menuValue<int>("Off",0),
  new menuValue<int>("Short",1),
  new menuValue<int>("Medium",2),
  new menuValue<int>("Long",3)
};
choose<int>& durMenu =*new choose<int>("Duration",duration,sizeof(durData)/sizeof(prompt*),durData);

//select field and options -------------------------------------
enum Fxs {Fx0,Fx1,Fx2} selFx;//target var
prompt* fxData[]={
  new menuValue<Fxs>("Default",Fx0),
  new menuValue<Fxs>("Pop",Fx1),
  new menuValue<Fxs>("Rock",Fx2)
};
select<Fxs>& fxMenu =*new select<Fxs>("Fx",selFx,sizeof(fxData)/sizeof(prompt*),fxData);

//toggle field and options -------------------------------------
bool led=false;//target var
void setLed() {digitalWrite(LED,!led);}
prompt* togData[]={
  new menuValue<bool>("On",true),
  new menuValue<bool>("Off",false)
};
toggle<bool>& ledMenu
  =*new toggle<bool>("LED:",led,sizeof(togData)/sizeof(prompt*),togData,(callback)setLed,enterEvent);

//the submenu -------------------------------------
prompt* subData[]={
  new prompt("Sub1"),
  new prompt("Sub2"),
  new Exit("<Back")
};
menuNode& subMenu=*new menuNode("sub-menu",sizeof(subData)/sizeof(prompt*),subData);

uint16_t year=2017;
uint16_t month=10;
uint16_t day=7;

//pad menu --------------------
prompt* padData[]={
  new menuField<typeof(year)>(year,"","",1900,3000,20,1,doNothing,noEvent),
  new menuField<typeof(month)>(month,"/","",1,12,1,0,doNothing,noEvent),
  new menuField<typeof(day)>(day,"/","",1,31,1,0,doNothing,noEvent)
};
menuNode& padMenu=*new menuNode(
  "Date",
  sizeof(padData)/sizeof(prompt*),
  padData,
  doNothing,
  noEvent,
  noStyle,
  (systemStyles)(_asPad|Menu::_menuData|Menu::_canNav|_parentDraw)
);

//the main menu -------------------------------------
void op1Func() {Serial.println("Op 1 executed");}
uint8_t test=55;//target var for numerical range field

//edit text field info
char* constMEM hexDigit MEMMODE="0123456789ABCDEF";//a text table
char* constMEM hexNr[] MEMMODE={"0","x",hexDigit,hexDigit};//text validators
char buf1[]="0x11";//text edit target

prompt* mainData[]={
  new prompt("Op 1",(callback)op1Func,enterEvent),
  new prompt("Op 2"),//we can set/change text, function and event mask latter
  new menuField<typeof(test)>(test,"Bright","",0,255,10,1,doNothing,noEvent),
  new textField("Addr",buf1,4,hexNr),
  &subMenu,
  &durMenu,
  &fxMenu,
  &ledMenu,
  &padMenu,
  new Exit("<Exit.")
};
menuNode& mainMenu=*new menuNode("Main menu",sizeof(mainData)/sizeof(prompt*),mainData/*,doNothing,noEvent,wrapStyle*/);

#define MAX_DEPTH 3

//input -------------------------------------
serialIn in(Serial);

//serial output -------------------------------------
idx_t tops[MAX_DEPTH];
constMEM panel panelsDef[] MEMMODE={{41,0,39,10}};
navNode* pnodes[sizeof(panelsDef)/sizeof(panel)];
panelsList panels(
  panelsDef,
  pnodes,
  sizeof(panelsDef)/sizeof(panel)
);
ansiSerialOut out(Serial,colors,tops,panels);

//outputs -------------------------------------
menuOut* outList[]={&out};
outputsList outs(outList,sizeof(outList)/sizeof(menuOut*));

//navigation control -------------------------------------
navNode path[MAX_DEPTH];
navRoot nav(mainMenu,path,MAX_DEPTH,in,outs);

void setup() {
  pinMode(LED,OUTPUT);
  setLed();
  Serial.begin(115200);
  while(!Serial);
  Serial.println("press any key...");
  while(!Serial.available());
  Serial.println("menu 4.x test");
  Serial.flush();
  mainMenu[0].shadow->text="Changed";
  Serial.println(mainMenu[0].getText());
  Serial<<"Sz:"<<mainMenu.sz()<<" "<<(sizeof(mainData)/sizeof(prompt*))<<endl;
}

void promptStatus(prompt& p,int x,int y) {
  out //<<ANSI::fill(x,y,40-x,y,' ')
      <<ANSI::xy(x,y)
      <<"prompt:";
  print_P(out, p.getText());
  out<<" "<<(long)&p<<endl;
}

void nodeStatus(navNode&nav,int i,int x,int y) {
  out //<<ANSI::fill(x,y,40-x,y,' ')
      <<ANSI::xy(x,y)
      <<"navNode:"<<i
      //<<" "<<(long)&nav
      <<" sel:"<<nav.sel
      <<" top:"<<out.tops[nav.root->level+(nav.target->has(_parentDraw)?-1:0)];
      //<<" changed:"<<(nav.target?nav.target->changed(nav, out, true):'?');
  if (nav.target) promptStatus(*nav.target,x,y+1);
  //else out<<ANSI::fill(x,y+1,40-x,y+1,' ');
}

void rootStatus(navRoot& nav,int x,int y) {
  out //<<ANSI::fill(x,y,40-x,y+1,' ')
      <<ANSI::xy(x,y)
      <<"navRoot: "<<(long)&nav
      <<" level:"<<nav.level
      <<ANSI::xy(x,y+1)
      <<"navFocus: "
      <<(long)&nav.navFocus;
  for(int n=0;n<MAX_DEPTH;n++)
    if(n<=nav.level) nodeStatus(path[n],n,x,y+3*(n+1));
    //else out<<ANSI::fill(x,y,40-x,y+3*(n+1),' ');
}

void debugStatus(navRoot& nav) {
  out <<ANSI::setForegroundColor(WHITE)
      <<ANSI::setBackgroundColor(BLACK)
      <<ANSI::fill(0,0,40,10);
  rootStatus(nav,1,1);
}

void loop() {
  nav.doInput();
  //debugStatus(nav);
  nav.doOutput();
  //nav.poll();//this device only draws when needed
  if(nav.sleepTask) {
    Serial.println();
    Serial.println("menu is suspended");
    Serial.println("presse [select] to resume.");
  }
  delay(200);//simulate a delay when other tasks are done
}
