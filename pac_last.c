#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <curses.h>
#include <ncurses.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include "pacman.h"


int set_up();
void reset();
void next_level();
void wrap_up();
void set_ticker();
void pac_move();
void ghost_move(int);
void ghost_exit(int);
void floyd_warshall();
int epistrofi_anaparastasis(int,int);
void Move();
void extract(int);

//define function for managing sigstop and siginterrupt
static volatile sig_atomic_t doneflag=0;
static void setdoneflag(int signum){endwin();initscr();clear();cbreak();doneflag=1;}

struct pacman pacy;
struct ghost ghosts[4];  //array containing the ghosts
char **pista;
char ch;
int **perase;
int D[10000][10000],P[10000][10000];                              //pinakes gitniasis,predecessor
struct akeraioi_warshall AW[10000];                               //ligo mpakaliki desmeusi alla gia dinamiki desmeusi pinaka domon ipirxe problima..
int rows,cols,myfd,dots=0,ticks=0,g_period_decr,fl_pil=0,power_mode=5,begin=0;  //ghost period decrement
int out_of_house=0,in_house=50,house_empty=0,gh_first_x,gh_first_y,diastasi=0,new_diastasi,pac_eats_ghost=0,eaten_ghosts=0;

main(int argc,char* argv[]){
	
	int i,j,count;
	rows=0;
	cols=0;
	struct sigaction act_move;
	struct sigaction c_action;
	struct sigaction z_action;

	
	//check correct use in stdin
	if(argc!=5){
		printf("Incorrect usage...Please retry!!!\n");
		return 1;
	}
	
	//assign pacman's period
	pacy.period=atoi(argv[2]);
	
	//assign ghosts' period
	for(i=0;i<4;i++)
		ghosts[i].period=atoi(argv[3]);
	
	//assign decrement of ghosts' movement
	g_period_decr=atoi(argv[4]);
	//open file 
	if((myfd=open(argv[1],O_RDONLY))==-1){
		perror("FAILED TO READ FROM pista.txt");
		return 1;
	}
			
	if(set_up()==1)
		return 1;
		
	//manage signals for exiting program
	c_action.sa_handler=setdoneflag;
	z_action.sa_handler=setdoneflag;
	c_action.sa_flags=0;
	z_action.sa_flags=0;
	if(sigemptyset(&c_action.sa_mask)==-1 || sigaction(SIGINT,&c_action,NULL)==-1){
		perror("Failed to asscociate Ctrl-C key combination with stoping program");
		return 1;
		
	}
	if(sigemptyset(&z_action.sa_mask)==-1 || sigaction(SIGTSTP,&z_action,NULL)==-1){
		perror("Failed to associate Ctrl-Z key combination with stoping program");
		return 1;
	}

	//manage signals for moving pacman and ghosts
	act_move.sa_handler=Move;
	act_move.sa_flags=0;
	
	if(sigemptyset(&act_move.sa_mask)==-1 || sigaction(SIGALRM,&act_move,NULL)==-1)
		perror("Failed to associate SIGALRM with function pac_move()");  
	
	
	//BASIC LOOP
	while(count=read(0,&ch,sizeof(char))){ 
		
		if(!(count==-1 && errno==EINTR)){
			if(doneflag==1 || pacy.lifes==0 || ghosts[1].period<=0){
				endwin();
				clear();            //gia pliri termatismo kai katharismo othonis apetite kai i pliktrologisi enos 
				if(pacy.lifes==0)   //opoiodipote pliktrou
					printf("You lost all your lifes...GAME OVER\n");
				if(ghosts[1].period<=0)
					printf("Ghosts' period have become negative..you cannot play anymore\n");
				return 1;
			}
			if(ch=='Q'){
					wrap_up();
					return 1; 
			}	    //must quit program
			else if(ch!='S' && begin==0)
				continue;
			else{           //read directions
				begin=1;
				if(ch=='R'){
					pacy.y_dir=-1;
					pacy.x_dir=0;
				}
				else if(ch=='F'){
					pacy.y_dir=+1;
					pacy.x_dir=0;
				}
				else if(ch=='D'){
					pacy.y_dir=0;
					pacy.x_dir=-1;
				}
				else if(ch=='G'){
					pacy.y_dir=0;
					pacy.x_dir=+1;
				}
				else if(ch=='N'){       //go to next level 
					next_level();
					begin=0;
				}
				else 
					continue;
			}
		}
	}

	wrap_up();
}            //end of main

int set_up()    //initialize structure and other stuff
{
	int stop=0,i,j,l=0,pos1,pos2;
	
	//initialize pacman
	pacy.symbol=PACMAN;
	pacy.x_dir=0;     //initialize directions
	pacy.y_dir=0;
	pacy.lifes=3;	  //initialize lifes
	pacy.dots=0;      //dots eaten
	pacy.score=0;
	
		
	//read from opened file and define rows and columns included
	while(read(myfd,&ch,sizeof(char)) && ch!=EOF){
		if(ch=='\n'){
			stop=1;
			rows++;
		}
		if(stop==0)
			cols++;
	}
	
	
	//allocate memory for array pista
	pista=malloc(rows*sizeof(char*));
	if(pista==NULL){
		printf("Cannot allocate memory\n");
		return 1;
	}
	for(i=0;i<rows;i++){
		*(pista+i)=malloc(cols*sizeof(char));
		if(*(pista+i)==NULL){
			printf("Cannot allocate memory\n");
			return 1;
		}
	}
	
	//return pointer myfd to the begining of the file and construct **pista
	lseek(myfd,0,0);
	for(i=0;i<rows;i++){
		for(j=0;j<cols;j++){
			read(myfd,&ch,sizeof(char));
			pista[i][j]=ch;
		}
		read(myfd,&ch,sizeof(char));		
	}		

	//define position of pacman and house gateaway in level
	for(i=0;i<rows;i++){
		for(j=0;j<cols;j++){
			if(pista[i][j]=='C'){      //pacman position
				pacy.x_pos=j;
				pacy.y_pos=i;
				pacy.x_init=j;
				pacy.y_init=i;
			}
			if(pista[i][j]==DOT)        //count dots
				dots++;
			if(pista[i][j]=='w'){
				gh_first_x=j;
				gh_first_y=i-1;
			}

		}
	}
	
	
	//allocate memory for array perase
	perase=malloc(rows*sizeof(int*));
	if(perase==NULL){
		printf("Cannot allocate memory\n");
		return 1;
	}
	for(i=0;i<rows;i++){
		*(perase+i)=malloc(cols*sizeof(int));
		if(*(perase+i)==NULL){
			printf("Cannot allocate memory\n");
			return 1;
		}
	}
	
	
	//find ghosts in level
	for(i=0;i<rows;i++)
	for(j=0;j<cols;j++){
		if(pista[i][j]=='G'){
			ghosts[0].x_pos=ghosts[0].x_init=j;
			ghosts[0].y_pos=ghosts[0].y_init=i;
			ghosts[1].x_pos=ghosts[1].x_init=j+1;
			ghosts[1].y_pos=ghosts[1].y_init=i;
			ghosts[2].x_pos=ghosts[2].x_init=j+2;
			ghosts[2].y_pos=ghosts[2].y_init=i;
			ghosts[3].x_pos=ghosts[3].x_init=j+3;
			ghosts[3].y_pos=ghosts[3].y_init=i;
			break;
		}
	}

	
	//initialize perase array
	for(i=0;i<rows;i++)
		for(j=0;j<cols;j++)
			perase[i][j]=0;

	for(i=0;i<rows;i++)
		for(j=0;j<cols;j++)
			if(pista[i][j]!=WALL && pista[i][j]!=GHOST && pista[i][j]!=BLANK && pista[i][j]!='w')
				diastasi++;      //brisko to megethos tou monodiastatou pinaka pou tha periexei tous akeraious gia ti thesi ton simeion pou epitrepetai i kinisi(floyd-warshall)
		
		
	for(i=0;i<rows;i++)            //kataskeui autou tou monodiastatou pinaka domon
		for(j=0;j<cols;j++){
				if(pista[i][j]!=WALL && pista[i][j]!=GHOST && pista[i][j]!=BLANK && pista[i][j]!='w'){
					AW[l].akeraios=i*cols+j;
					AW[l].x_pos=j;
					AW[l].y_pos=i;
					l++;      //to l pairnei timi mexri diastasi-1
				}
				if(l>=diastasi)
					break;
		}
		
		
		
	new_diastasi=AW[diastasi-1].akeraios;  //trick!!
	
	for(i=0;i<=new_diastasi;i++)        //arxikopoiisi
		for(j=0;j<=new_diastasi;j++){
			if(i==j)
				D[i][j]=0;
			else
				D[i][j]=3333;    //kati poli megalo gia na min isxiei i if tis floyd_warshall
			P[i][j]=-1;
		}
		
	for(i=0;i<rows;i++)            //oi akmes na exoun baros 1
		for(j=0;j<cols;j++){
			if(pista[i][j]==DOT && j==cols-1){
				pos1=epistrofi_anaparastasis(i,j);
				pos2=epistrofi_anaparastasis(i,0);
				D[pos1][pos2]=1;
				D[pos2][pos1]=1;
				P[pos1][pos2]=pos1;
				P[pos2][pos1]=pos2;
			}
			else if(pista[i][j]==DOT || pista[i][j]==PACMAN || pista[i][j]==POW_PIL){
				if(pista[i+1][j]==DOT || pista[i+1][j]==PACMAN || pista[i+1][j]==POW_PIL){
					pos1=epistrofi_anaparastasis(i,j);
					pos2=epistrofi_anaparastasis(i+1,j);
					D[pos1][pos2]=1;
					D[pos2][pos1]=1;
					P[pos1][pos2]=pos1;
					P[pos2][pos1]=pos2;
				}
				if(pista[i-1][j]==DOT || pista[i-1][j]==PACMAN || pista[i-1][j]==POW_PIL){
					pos1=epistrofi_anaparastasis(i,j);
					pos2=epistrofi_anaparastasis(i-1,j);
					D[pos1][pos2]=1;
					D[pos2][pos1]=1;
					P[pos1][pos2]=pos1;
					P[pos2][pos1]=pos2;
				}
				if(pista[i][j+1]==DOT || pista[i][j+1]==PACMAN || pista[i][j+1]==POW_PIL){
					pos1=epistrofi_anaparastasis(i,j);
					pos2=epistrofi_anaparastasis(i,j+1);
					D[pos1][pos2]=1;
					D[pos2][pos1]=1;
					P[pos1][pos2]=pos1;
					P[pos2][pos1]=pos2;
				}
				if(pista[i][j-1]==DOT || pista[i][j-1]==PACMAN || pista[i][j-1]==POW_PIL){
					pos1=epistrofi_anaparastasis(i,j);
					pos2=epistrofi_anaparastasis(i,j-1);
					D[pos1][pos2]=1;
					D[pos2][pos1]=1;
					P[pos1][pos2]=pos1;
					P[pos2][pos1]=pos2;
				}
			}
		}
		
	floyd_warshall();
	
		
	
	initscr();
	if(has_colors()==FALSE)         //COLOURS
	{
		endwin();
		printf("your terminal does not support colour\n");
		exit(1);
	}
	start_color();
	init_pair(1,COLOR_RED,COLOR_BLACK);    //colour pair for pacman
	init_pair(2,COLOR_GREEN,COLOR_MAGENTA); //colour pair for ghosts
	
	
	//make **pista appear on screen
	for(i=0;i<rows;i++)
		for(j=0;j<cols;j++){
			if(pista[i][j]==pista[pacy.y_pos][pacy.x_pos]){
				standout();
				mvaddch(i,j,pista[i][j]);
				standend();
			}
			else
				mvaddch(i,j,pista[i][j]);
		}
	move(LINES-1,COLS-1);
	refresh();
	
	set_ticker();
	noecho();
	crmode();
	
}      //end of setup

void Move(int signum){
	int i;
	ticks++;
	char buffer[5];
	
	//in how many ticks will we move pacman
	if(ticks%pacy.period==0)
		pac_move();
	
	//in how many ticks will we extract a ghost out of the house
	if(ticks%60==0 && house_empty==0 && begin==1){
		ghost_exit(out_of_house);
		out_of_house++;
	}
	
	//in how many ticks will we move a ghost and check if ghost eating mode is enabled
	if(ticks%ghosts[0].period==0)
		if(pac_eats_ghost==0)
			for(i=0;i<out_of_house;i++)
				ghost_move(i);
		else{
			ghost_exit(in_house);
			for(i=0;i<4;i++)
				if(i!=in_house)
					ghost_move(i);
			pac_eats_ghost=0;
				
		}
	
			
	//when we extract all ghosts from the house we just want to move them,
	//so we assign value 0 to the flag "house empty"
	if(out_of_house==4)
		house_empty=1;
	
	
	//how many ticks will the power mode lust
	if(fl_pil && ticks%25==0){
		mvaddstr(rows+6,0,"END OF POWER MODE IN:");
		sprintf(buffer,"%d",power_mode);
		mvaddstr(rows+6,21,buffer);
		power_mode--;
		refresh();
	}
	
	//if power mode is over return to normal dot-eating
	if(power_mode<0){
		mvaddstr(rows+6,0,"                        ");
		fl_pil=0;
		power_mode=5;
		refresh();
	}

}

void ghost_move(int i){

	int y_cur,x_cur,moved=0,nodes,ghost_akeraios,pac_akeraios,j,k,kill=0,pac_pil,max_distance;
	
		y_cur=ghosts[i].y_pos;
		x_cur=ghosts[i].x_pos;
		
	ghost_akeraios=epistrofi_anaparastasis(y_cur,x_cur);          //brisei ti thesi tou ghost se morfi i*cols+j
	pac_akeraios=epistrofi_anaparastasis(pacy.y_pos,pacy.x_pos);  //briskei ti thesi tou pac se morfi i*cols+j
	if(fl_pil==0){
		nodes=D[ghost_akeraios][pac_akeraios];                    //to mikos tou elaxistou monopatiou metaxi tou ghost[i] kai tou pac
		int TEMP[nodes-1];                                        //apothikeusi ton endiameson korifon plithous nodes-1 afou kathe akmi exei baros 1
		if(nodes==1){                                             //apexoun mia akmi
			ghosts[i].y_pos=pacy.y_pos;
			ghosts[i].x_pos=pacy.x_pos;
			moved=1;
			kill=1;
			begin=0;                   
		}
		else{
			for(j=nodes-2;j>=0;j--){  
				if(j==nodes-2)
					TEMP[j]=P[ghost_akeraios][pac_akeraios];
				else{
					k=TEMP[j+1];
					TEMP[j]=P[ghost_akeraios][k];
					}
			}
			for(j=0;j<diastasi;j++)
				if(AW[j].akeraios==TEMP[0]){                        //pigene to ghost stin proti korifi tou elaxistou monopatiou
					ghosts[i].y_pos=AW[j].y_pos;
					ghosts[i].x_pos=AW[j].x_pos;
					moved=1;
					break;
				}
		}
	} 
	else{                                                           //briskomaste se katastasi xapiou!!
		pac_pil=epistrofi_anaparastasis(pacy.y_pos,pacy.x_pos);
		max_distance=AW[0].akeraios;
		for(j=0;j<diastasi;j++)                                      //euresi tis megaliteris apostasis se akeraia anaparastasi apo ton pacman
			if(abs(pac_pil-max_distance)<abs(pac_pil-AW[j].akeraios))  //thelo apoliti timi afou einai apostasi
				max_distance=AW[j].akeraios;
				
		nodes=D[ghost_akeraios][max_distance];
		int TEMP[nodes-1];
		if(nodes==1){
			ghosts[i].y_pos=AW[j].y_pos;
			ghosts[i].x_pos=AW[j].x_pos;
			moved=1;
		}
		else{
			for(j=nodes-2;j>=0;j--){  
				if(j==nodes-2)
					TEMP[j]=P[ghost_akeraios][max_distance];
				else{
					k=TEMP[j+1];
					TEMP[j]=P[ghost_akeraios][k];
					}
			}
			for(j=0;j<diastasi;j++)
				if(AW[j].akeraios==TEMP[0]){                          //pigene to ghost stin proti korifi tou elaxistou monopatiou
					ghosts[i].y_pos=AW[j].y_pos;
					ghosts[i].x_pos=AW[j].x_pos;
					moved=1;
					break;
				}


						
		}	
	}
	if(moved){
        
		if(perase[y_cur][x_cur]==0)
			mvaddch(y_cur,x_cur,DOT);
		else
			mvaddch(y_cur,x_cur,BLANK);
		standout();
		attron(COLOR_PAIR(2));
		mvaddch(ghosts[i].y_pos,ghosts[i].x_pos,GHOST);
		standend();
		attroff(COLOR_PAIR(2));
	}

	
	if(kill){
		pacy.lifes--;
		sleep(2);                                                   //case coincides with pacman eating ghost,so we're gonna have 2 seconds here 
		                                                            //and 2 secs in pacmove sp we're ok
		reset();      
	}
	

}

void ghost_exit(int i){

	standout();
	mvaddch(gh_first_y,gh_first_x,GHOST);
	standend();
	mvaddch(ghosts[i].y_init,ghosts[i].x_init,BLANK);
	ghosts[i].y_pos=gh_first_y;
	ghosts[i].x_pos=gh_first_x;
}

void floyd_warshall(){

	int i,j,k;

	for(k=0;k<=new_diastasi;k++)
		for(i=0;i<=new_diastasi;i++)
			for(j=0;j<=new_diastasi;j++)
				if(D[i][k]+D[k][j]<D[i][j]){
					D[i][j]=D[i][k]+D[k][j];
					P[i][j]=P[k][j];
				}
	
}

int epistrofi_anaparastasis(int r,int c){                             //pairnei tis sintetagmenes tis pistas kai epistrefei ton antisoixo akeraio apo to struct AW

	int i;
	for(i=0;i<diastasi;i++)
		if(AW[i].y_pos==r && AW[i].x_pos==c)
			return AW[i].akeraios;
	
}


void pac_move(){
	int x_cur,y_cur,moved=0,flag=0,fl_score=0,i;
	char buffer[5];
	
	x_cur=pacy.x_pos;
	y_cur=pacy.y_pos;
	
	if(pista[y_cur+pacy.y_dir][x_cur+pacy.x_dir]==WALL){
		x_cur=pacy.x_pos;
		y_cur=pacy.y_pos;
	}					                                               //den kanei tpt proxoraei parakato
	if((pista[y_cur+pacy.y_dir][x_cur+pacy.x_dir]==DOT || pista[y_cur+pacy.y_dir][x_cur+pacy.x_dir]==PACMAN) && x_cur+pacy.x_dir<=cols-1 && x_cur+pacy.x_dir>=0){    //na einai sta oria tou pinaka
		pacy.x_pos+=pacy.x_dir;
		pacy.y_pos+=pacy.y_dir;
		moved=1;
		if(pista[y_cur+pacy.y_dir][x_cur+pacy.x_dir]==DOT && perase[y_cur+pacy.y_dir][x_cur+pacy.x_dir]==0){
			pacy.score=pacy.score+10;
			dots--;
			perase[y_cur+pacy.y_dir][x_cur+pacy.x_dir]=1;
		}
		flag=1;
	}
	if(pacy.x_pos==cols-1 && pacy.x_dir==+1 && flag==0){                 //flag:na min mpainei edo kai kato an exei mpei sto proigoumeno for
		pacy.x_pos=0;
		moved=1;
		if(perase[y_cur+pacy.y_dir][cols-1]==0){
			pacy.score=pacy.score+10;
			perase[y_cur+pacy.y_dir][cols-1]=1;
		}
	}
	if(pacy.x_pos==0 && pacy.x_dir==-1 && flag==0){
		pacy.x_pos=cols-1;
		moved=1;
		if(perase[y_cur+pacy.y_dir][0]==0){
			pacy.score=pacy.score+10;
			perase[y_cur+pacy.y_dir][0]=1;
		}
	}
	
	//i antistrofi metrisi gia tin katastasi dynamis tha ginei me sleep
	if(pista[y_cur+pacy.y_dir][x_cur+pacy.x_dir]==POW_PIL){    
		pacy.x_pos+=pacy.x_dir;
		pacy.y_pos+=pacy.y_dir;
		moved=1;
		if(perase[y_cur+pacy.y_dir][x_cur+pacy.x_dir]==0){
			pacy.score=pacy.score+60;
			fl_pil=1;
			perase[y_cur+pacy.y_dir][x_cur+pacy.x_dir]=1;
		}
	}
	
	if(fl_pil==1)
		for(i=0;i<4;i++)
			if(pacy.y_pos==ghosts[i].y_pos && pacy.x_pos==ghosts[i].x_pos){
				ghosts[i].x_pos=ghosts[i].x_init;
				ghosts[i].y_pos=ghosts[i].y_init;
				perase[ghosts[i].y_init][ghosts[i].x_init]=1;          //so as not to print dots inside ghost house
				in_house=i;                                            //so as to extract the inside-house ghost from the house
				pac_eats_ghost=1;                                      //flag for knowing what to do in Move()
				eaten_ghosts++;                                        //flag for manipulating pacman's score
				if(eaten_ghosts==1)
					pacy.score+=300;
				else if(eaten_ghosts==2)
					pacy.score+=600;
				else if(eaten_ghosts==3)
					pacy.score+=900;
				else
					pacy.score+=3200;
				break;
			}

	
	if(moved){
	
		mvaddch(y_cur,x_cur,BLANK);
		standout();
		attron(COLOR_PAIR(1));
		mvaddch(pacy.y_pos,pacy.x_pos,pacy.symbol);
		if(pac_eats_ghost==1){
			mvaddch(ghosts[i].y_init,ghosts[i].x_init,GHOST);
			sleep(2);
		}
		attroff(COLOR_PAIR(1));
		standend();
	}
	
	mvaddstr(rows+1,0,"SCORE:");
	sprintf(buffer,"%d",pacy.score);
	mvaddstr(rows+1,6,buffer);
	//////////////////////////////////////////////////
	mvaddstr(rows+2,0,"LIFES:");
	sprintf(buffer,"%d",pacy.lifes);
	mvaddstr(rows+2,6,buffer);
	//////////////////////////////////////////////////
	mvaddstr(rows+3,0,"DOTS REMAINING:");
	sprintf(buffer,"%d",dots);
	mvaddstr(rows+3,15,buffer);
	if(dots<100){
		mvaddch(rows+3,17,BLANK);
		if(dots<10)
			mvaddch(rows+3,16,BLANK);
	}
	///////////////////////////////////////////////////
	mvaddstr(rows+4,0,"PACMAN'S MOVEMENT PERIOD:");
	sprintf(buffer,"%d",pacy.period);
	mvaddstr(rows+4,25,buffer);
	//////////////////////////////////////////////////
	mvaddstr(rows+5,0,"GHOSTS' MOVEMENT PERIOD:");
	sprintf(buffer,"%d",ghosts[1].period);
	mvaddstr(rows+5,24,buffer);
	if(ghosts[0].period<10)
		mvaddch(rows+5,25,BLANK);
	
	
	move(LINES-1,COLS-1);
	refresh();
	
	if(dots==0){
		pacy.score+=5000;
		begin=0;
		next_level();
	}
}


void next_level(){ 
	int i;
	
	for(i=0;i<4;i++){
		ghost_exit(i);
		ghosts[i].period=ghosts[i].period-g_period_decr;
	}
	reset();                              //reset level to the initiale state
}

void reset(){
	int i,j;
	house_empty=0;
	out_of_house=0;
	pacy.x_pos=pacy.x_init;
	pacy.y_pos=pacy.y_init;
	pacy.x_dir=0;
	pacy.y_dir=0;
	
	dots=0;
	for(i=0;i<rows;i++)
		for(j=0;j<cols;j++)
			if(pista[i][j]==DOT)        //count dots
				dots++;
	
	//reinitialize ghosts,to eixame kanei me mia etabliti dots_init alla den douleue 
	//opote tis ksanametrame
	for(i=0;i<4;i++){
		ghosts[i].x_pos=ghosts[i].x_init;
		ghosts[i].y_pos=ghosts[i].y_init;
	}
	
	//reinitialize perase array
	for(i=0;i<rows;i++)
		for(j=0;j<cols;j++)
			perase[i][j]=0;

	//reinitalize pista
	for(i=0;i<rows;i++)
		for(j=0;j<cols;j++){
			if(pista[i][j]==pista[pacy.y_init][pacy.x_init]){
				standout();
				mvaddch(i,j,pista[i][j]);
				standend();
			}
			else
				mvaddch(i,j,pista[i][j]);
		}
	move(LINES-1,COLS-1);
}
	
//function for exiting normally the program
void wrap_up()
{
	int i,j;
	//free pista
	for(i=0;i<rows;i++)
		free(*(pista+i)); 
	free(pista);
	
	//free perase
	for(i=0;i<rows;i++)
		free(*(perase+i)); 
	free(perase);
	
	set_ticker( 0 );    //stop ticker
	close(myfd);
	endwin();		// put back to normal	
}

//TIMER
void set_ticker() {
        struct itimerval new_timeset;

		//define the first timer to set off after 40,000 microseconds
        new_timeset.it_interval.tv_sec  = 0;        /* set reload       */
        new_timeset.it_interval.tv_usec = 40000;      /* new ticker value */
        
		//define the interval between each two timer set-offs to be 40.000 microseconds as well
		new_timeset.it_value.tv_sec     = 0  ;      /* store this       */
        new_timeset.it_value.tv_usec    = 40000;     /* and this         */

		setitimer(ITIMER_REAL, &new_timeset,0);
}


	
	
	
	
	
	
	