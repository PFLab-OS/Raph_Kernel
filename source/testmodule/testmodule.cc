int main(int argc, char *argv[])
{
  if(argv && argv[0]){
    argv[0][0] = 'X';
    argv[0][1] = 0;
  }
  return 0;
}
