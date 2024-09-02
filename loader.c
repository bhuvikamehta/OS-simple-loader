#include "loader.h"
#include <stdbool.h>

// ehdr= ELF Header, phdr= Program Header
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  free(phdr);
  free(ehdr);  
}

// check whether the file is a valid ELF file
bool elfFileCheck(Elf32_Ehdr *ehdr){
  char magicNo[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};
  if(ehdr->e_ident[0]==magicNo[0] && ehdr->e_ident[1]==magicNo[1] && ehdr->e_ident[2]==magicNo[2] && ehdr->e_ident[3]==magicNo[3]){
    return true;
  }
  else{
    return false;
  }
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd = open(*exe, O_RDONLY);
  if (fd < 0){
    printf("Error opening the file\n");
    exit(1);
  }

  // 1. Load entire binary content into the memory from the ELF file.

  int eof = lseek(fd, 0, SEEK_END); // determines the file size
  int lseek_result = lseek(fd, 0, SEEK_SET); // resets the pointer back to beginning
  if (lseek_result < 0){
    printf("lseek() was not successful\n");
    exit(1);
  }
  char* bin_data = (char*)malloc(eof+1);
  int read_result = read(fd, bin_data, eof); // reads ELF file content into bin_data
  if (read_result < 0){
    printf("Error reading the file\n");
    exit(1);
  }

  // Reading ELF header
  // allocates memory for ehdr
  ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
  lseek_result = lseek(fd, 0, SEEK_SET);
  if (lseek_result < 0){
    printf("lseek() was not successful\n");
    exit(1);
  }
  // reads ELF header into ehdr
  read_result = read(fd, ehdr, sizeof(Elf32_Ehdr));
  if (read_result < 0){
    printf("Error reading the file\n");
    exit(1);
  }

  bool valid_elffile = elfFileCheck(ehdr);
  if (valid_elffile == false){
    printf("It is not a valid ELF file\n");
    exit(1);
  }

  // Reading ELF program header
  // allocates memory for phdr
  phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
  lseek_result = lseek(fd, ehdr->e_phoff, SEEK_SET);
  if (lseek_result < 0){
    printf("lseek() was not successful\n");
    exit(1);
  }
  // reads the program header table into phdr
  read_result = read(fd, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum);
  if (read_result < 0){
    printf("Error reading the file\n");
    exit(1);
  }



  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  Elf32_Phdr *required_phdr;
  int i=0;
  while(i < ehdr->e_phnum){
    if (phdr[i].p_type == PT_LOAD && ehdr->e_entry>=phdr[i].p_vaddr && ehdr->e_entry < phdr[i].p_vaddr+phdr[i].p_memsz){
      required_phdr = &phdr[i];
      break;
    }
    i++;
  }

  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  Elf32_Phdr *virtual_mem = mmap(NULL, required_phdr->p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0,0);
  lseek_result = lseek(fd, required_phdr->p_offset, SEEK_SET);
  if (lseek_result < 0){
    printf("lseek() was not successful\n");
    exit(1);
  }
  // loads the segment into the allocated memory
  read_result = read(fd, virtual_mem, required_phdr->p_memsz);
  if (read_result < 0){
    printf("Error reading the file\n");
    exit(1);
  }

  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step

  void *entrypoint = (void *)virtual_mem + (ehdr->e_entry - required_phdr->p_vaddr);

  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.

  int (_start)() = (int ()())entrypoint;

  // 6. Call the "_start" method and print the value returned from the "_start"

  int result = _start();
  printf("User _start return value = %d\n", result);

  if (munmap(virtual_mem, required_phdr->p_memsz) < 0){
    printf("Error unmapping");
  }
  free(bin_data); 
  close(fd);
}

int main(int argc, char** argv){
  // checks whether only one argument is passed
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file

  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(&argv[1]);

  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  
  return 0;
}
