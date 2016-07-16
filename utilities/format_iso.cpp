#include<iostream>
#include<fstream>
#include<string.h>
#include<math.h>
#include<stdint.h>
using namespace std;

bool find_record(const char* directory, const int dir_size, const char* record_name, int& record_sector, int& record_size)
{
	bool record_found = false;
	int dir_seekg = 0x44;			//Skip first 0x44 bytes, they are directory records for current and parent directory
	while((dir_seekg < dir_size) && (*((char*)(directory + dir_seekg)) > 0))	//Check if we are past the directory extent or all records have been traversed
	{
		int dir_record_size = *((char*)(directory + dir_seekg));	//Get the size of the record in the directory extent
		int file_name_size = *((char*)(directory + dir_seekg + 32));	//Get length of the file name
		char *file_name = new char[file_name_size+1];				//Copy the record name in a buffer
		memcpy(file_name, directory + dir_seekg + 33, file_name_size);
		file_name[file_name_size] = 0;			//Add a null character at the end
		if(strcmp(file_name, record_name) == 0)			//If file name and the record name match
		{
			record_sector = *((int*)(directory + dir_seekg + 2));		//Return the parameters like
			record_size = *((int*)(directory + dir_seekg + 10));		//file size and file sector
			record_found = true;
			break;
		}
		delete[] file_name;
		dir_seekg+=dir_record_size;			//Move to next record in the directory extent
	}
	return record_found;
}

int main(int argc, char* argv[])
{
	if(argc!=2)	//Check if exactly 2 arguments i.e. EXE name and ISO file name are provided
	{
		cout<<"Supply only the disk ISO name"<<endl;
		return 1;
	}
	char *isoname = new char[strlen(argv[1])];	//Copy name of ISO file to be formatted in a buffer
	strcpy(isoname, argv[1]);
	fstream isofile;
	isofile.open(isoname, ios::in | ios::out | ios::binary);	//Open the ISO file
	if(!isofile.is_open())		//ISO file cannot be opened (failed)
	{
		cout<<"Wrong ISO file name"<<endl;
		isofile.close();
		return 1;
	}

	int volume_descriptor_address = 0x8000;		//Skip first 0x8000 (i.e. 32 sectors of ISO always empty according to ISO-9660
	char *sector = new char[2048];				// Create buffer for sector
	memset(sector,0,2048);
	char primary_volume_descriptor[] = {0x01,'C','D','0','0','1',0x01,0x00};	//These bytes define the sector as volume descriptor sector
	bool volume_descriptor_found = false;
	int root_directory_size = 0, root_directory_sector = 0;
	while(!volume_descriptor_found && !isofile.eof())
	{
		isofile.seekg(volume_descriptor_address,ios::beg);
		isofile.read(sector,2048);
		if(equal(primary_volume_descriptor,primary_volume_descriptor + 7,sector))
		{
			root_directory_sector = *((int*)(sector + 158));	//Get root directory extent sector and size
			root_directory_size = *((int*)(sector + 166));
			volume_descriptor_found = true;
		}
		else
		{
			volume_descriptor_address+=0x0800;	//If not found, move on to next sector in the ISO file
		}
	}
	delete[] sector;
	if(!volume_descriptor_found)
	{
		cout<<"Not a valid ISO file"<<endl;
		isofile.close();
		return 1;
	}

	char *root_directory = new char[root_directory_size];
	isofile.seekg(root_directory_sector * 0x800, ios::beg);
	isofile.read(root_directory, root_directory_size);
	int boot_dir_sector, boot_dir_size;
	if(!find_record(root_directory,root_directory_size,"BOOT",boot_dir_sector,boot_dir_size))
	{
		delete[] root_directory;
		isofile.close();
		cout<<"BOOT directory not found!"<<endl;
		return 1;
	}
	delete[] root_directory;

	char *boot_dir = new char[boot_dir_size];
	isofile.seekg(boot_dir_sector * 0x800, ios::beg);
	isofile.read(boot_dir,boot_dir_size);
	int stage1_dir_sector, stage1_dir_size, stage2_dir_sector, stage2_dir_size, kernel64_sector, kernel64_size;
	if(!find_record(boot_dir, boot_dir_size, "STAGE1", stage1_dir_sector,stage1_dir_size))
	{
		delete[] boot_dir;
		isofile.close();
		cout<<"STAGE1 directory not found!"<<endl;
		return 1;
	}
	if(!find_record(boot_dir, boot_dir_size, "STAGE2", stage2_dir_sector,stage2_dir_size))
	{
		delete[] boot_dir;
		isofile.close();
		cout<<"STAGE2 directory not found!"<<endl;
		return 1;
	}
	if(!find_record(boot_dir, boot_dir_size,"KERNEL.;1",kernel64_sector,kernel64_size))
	{
		delete[] boot_dir;
		isofile.close();
		cout<<"KERNEL not found!"<<endl;
		return 1;
	}
	delete[] boot_dir;

	char *stage1_dir = new char[stage1_dir_size];
	isofile.seekg(stage1_dir_sector * 0x800, ios::beg);
	isofile.read(stage1_dir, stage1_dir_size);
	int boot_bin_sector, boot_bin_size;
	if(!find_record(stage1_dir,stage1_dir_size,"BOOTLOAD.BIN;1",boot_bin_sector,boot_bin_size))
	{
		delete[] stage1_dir;
		isofile.close();
		cout<<"BOOTLOAD.BIN file not found!"<<endl;
		return 1;
	}
	char *stage2_dir = new char[stage2_dir_size];
	isofile.seekg(stage2_dir_sector * 0x800, ios::beg);
	isofile.read(stage2_dir, stage2_dir_size);
	char core_files[][15] = {"X64.BIN;1","MMAP.BIN;1","VIDMODES.BIN;1","A20.BIN;1","GDT.BIN;1","K64LOAD.BIN;1","ELFPARSE.BIN;1","LOADER32.;1","KERNEL.;1"};
	int core_file_sectors[9], core_file_sizes[9], core_file_segments[9];
	for(int i=0;i<5;i++)
	{
		if(!find_record(stage1_dir, stage1_dir_size, core_files[i], core_file_sectors[i], core_file_sizes[i]))
		{
			delete[] stage1_dir;
			isofile.close();
			cout<<core_files[i]<<" not found!"<<endl;
			return 1;
		}
	}
	for(int i=5;i<8;i++)
	{
		if(!find_record(stage2_dir, stage2_dir_size, core_files[i], core_file_sectors[i], core_file_sizes[i]))
		{
			delete[] stage2_dir;
			isofile.close();
			cout<<core_files[i]<<" not found!"<<endl;
			return 1;
		}
	}
	delete[] stage1_dir;
	delete[] stage2_dir;

	core_file_segments[0] = 0x0080;
	core_file_sectors[8] = kernel64_sector;
	core_file_sizes[8] = kernel64_size;
	for(int i=1;i<9;i++)
	{
		core_file_segments[i] = core_file_segments[i-1] + ceil((double)core_file_sizes[i-1]/16);
	}
	int boot_bin_seekp = boot_bin_sector * 0x800 + 32;	// Skip first 32 bytes (contains int 22h routine)
	for(int i=0;i<9;i++)
	{
		isofile.seekp(boot_bin_seekp + 2, ios::beg);	// Skip first 2 bytes of each DAP
		uint16_t number_of_sectors = ceil((double)core_file_sizes[i]/0x800); //DAP number of sectors to load
		isofile.write((char*)&number_of_sectors,2);
		uint16_t dap_offset = 0x0000;		//Offset at which data is loaded is always 0x00
		isofile.write((char*)&dap_offset, 2);
		if(strcmp(core_files[i],"ELFPARSE.BIN;1")==0)
		{
			uint16_t elfparse_segment = 0x2000;
			isofile.write((char*)&elfparse_segment,2);
		}
		else if(strcmp(core_files[i],"LOADER32.;1")==0 || strcmp(core_files[i],"KERNEL.;1")==0)
		{
			uint16_t kernel32elf_segment = 0x8000;
			isofile.write((char*)&kernel32elf_segment,2);
		}
		else
		{
			isofile.write((char*)&(core_file_segments[i]),2);	//Segment at which data is loaded
		}
		isofile.write((char*)&(core_file_sectors[i]),4);	//First sector of the file to be loaded
		boot_bin_seekp+=16;
	}

	cout<<"ISO formatted"<<endl;

	delete[] isoname;
	isofile.close();
	return 0;
}
