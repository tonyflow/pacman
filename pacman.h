#define PACMAN 'C'
#define GHOST 'G'
#define POW_PIL 'P'
#define WALL 'W'
#define BLANK ' '
#define DOT '.'

struct pacman{
	int x_pos,y_pos,
		x_dir,y_dir,
		lifes,dots,score,period,y_init,x_init;
	char symbol;
};
struct ghost{
	int x_pos,y_pos,
		period,y_init,x_init;
};

struct akeraioi_warshall{
	int akeraios,x_pos,y_pos;
};
