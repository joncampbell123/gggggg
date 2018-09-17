
typedef struct
{
  char		identification[4];	/* should be IWAD */
  int		numlumps  __PACKED__ ;
  int		infotableofs  __PACKED__ ;
}  __PACKED__  wadinfo_t;


typedef struct
{
  int		filepos;
  int		size;
  char		name[8];
}  __PACKED__  filelump_t;


