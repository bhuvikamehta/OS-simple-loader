#include "loader.h"

// ehdr = ELF Header, phdr = Program Header
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * Release memory and other cleanups
 */
void loader_cleanup() {
  free(phdr);
  free(ehdr);  
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** argv) {  
  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf("Error opening the file");
    printf("\n");
    exit(1);
  }

  // 1. Load entire binary content into the memory from the ELF file.

  int elf_size = lseek(fd, 0, SEEK_END); // Determine the file size
  int ptr = lseek(fd, 0, SEEK_SET); // Reset the pointer back to the beginning
  if (ptr < 0) {
    printf("ptr is not at beginning.");
    printf("\n");
    exit(1);
  }

  char* elf_data = (char*)malloc(elf_size);
  int read_length = read(fd, elf_data, elf_size); // Read ELF file content into elf_data
  if (read_length < 0) {  
    printf("could not read the file");
    printf("\n");
    exit(1);
  }

  // Reading ELF header
  ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr)); 
  ptr = lseek(fd, 0, SEEK_SET); // reset the ptr back to beginning.
  if (ptr < 0) {
    printf("ptr is not at beginning.");
    exit(1);
  }
  read_length = read(fd, ehdr, sizeof(Elf32_Ehdr)); // Read ELF header into elf_header_data
  if (read_length < 0) {
    printf("could not read the file");
    printf("\n");
    exit(1);
  }

  /*
  * Check whether the file is a valid ELF file
  */

  int check_file(Elf32_Ehdr *eehdr) {

    char elf_magicno[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};
    if ((ehdr->e_ident[0] == elf_magicno[0]) &&
        (ehdr->e_ident[1] == elf_magicno[1]) &&
        (ehdr->e_ident[2] == elf_magicno[2]) &&
        (ehdr->e_ident[3] == elf_magicno[3])){
            return 1; // true
        } 
    else {
        return 0; // false
    }
  }

  int valid_elf_file = check_file(ehdr);
  if (valid_elf_file == 0) {
    printf("Not a valid ELF File");
    printf("\n");
    exit(1);
  }

  // Reading ELF program header
  phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
  ptr = lseek(fd, ehdr->e_phoff, SEEK_SET);
  if (ptr < 0) {
    printf("lseek() command failed.");
    printf("\n");
    exit(1);
  }
  read_length = read(fd, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum); // Read the program header table into program_header_data
  if (read_length < 0) {
    printf("could not read the file");
    exit(1);
  }

  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c

  Elf32_Phdr *target_segment = NULL;
  int i = 0;
  while (i <ehdr->e_phnum) {
    if ((phdr[i].p_type == PT_LOAD) &&
        (ehdr->e_entry < phdr[i].p_vaddr + phdr[i].p_memsz)) 
        {
        target_segment = &phdr[i];
        break;
        }
    i++;
  }

  if (target_segment == NULL) {
    printf("PT_LOAD segment having the entrypoint is not present");
    printf("\n");
    exit(1);
  }

  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  void *virtual_mem = mmap(NULL, target_segment->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  if (virtual_mem == MAP_FAILED) {
    printf("Error mapping memory");
    printf("\n");
    exit(1);
  }

  ptr = lseek(fd, target_segment->p_offset, SEEK_SET);
  if (ptr < 0) {
    printf("Could not seek to the segment start");
    exit(1);
  }
  int read_segment_result = read(fd, virtual_mem, target_segment->p_memsz); // Load the segment into the allocated memory
  if (read_segment_result < 0) {
    printf("could not read the file");
    printf("\n");
    exit(1);
  }

  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  void *entrypoint = (void *)virtual_mem + (ehdr->e_entry - target_segment->p_vaddr);

  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  int (*_start)() = (int (*)())entrypoint;

  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = _start();
  printf("User _start return value = %d\n", result);

  if (munmap(virtual_mem, target_segment->p_memsz) < 0) {
    printf("Error unmapping\n");
  }
  free(elf_data); 
  close(fd);
}

int main(int argc, char** argv) {
  // Checks whether only one argument is passed
  if (argc != 2) {
    printf("Usage: %s <ELF Executable>\n", argv[0]);
    exit(1);
  }

  // 1. Carry out necessary checks on the input ELF file

  // 2. Passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);

  // 3. Invoke the cleanup routine inside the loader  
  loader_cleanup();

  return 0;
}
