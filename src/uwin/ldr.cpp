/*
 *  uwin layer PE file loader
 *
 *  Copyright (c) 2020 Nikita Strygin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "uwin/uwin.h"

#include "uwin/uwindows.h"
#include "uwin/uwinerr.h"
#include "uwin/util/align.h"
#include "uwin/util/mem.h"
#include "uwin/util/str.h"

#include <cstring>
#include "fcaseopen.h"

#include <cassert>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cctype>

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace std { // NOLINT(cert-dcl58-cpp)
    namespace filesystem = std::experimental::filesystem;
}
#else
#error "no filesystem support ='("
#endif


struct ldr_override
{
    const char* name;
    const uint8_t* data;
};

#define INCLUDE_LDR_OVERRIDES_H
// defines an `static struct ldr_override ldr_overrides[]`, built by build_dll_archive.py
#include "ldr_overrides.h"
#undef INCLUDE_LDR_OVERRIDES_H


struct module {
    std::string name;
    uint32_t image_base;
    uint32_t entry_point;
    uint32_t export_directory;
    uint32_t export_directory_size;
    uint32_t stack_size;
    uint32_t code_base;

    module(std::string name) : name(name) {}
};

struct library_init_routine {
    uint32_t hinst;
    uint32_t routine;

    library_init_routine(uint32_t hinst, uint32_t routine)
        : hinst(hinst), routine(routine) {}

    library_init_routine(module& module)
        : hinst(module.image_base), routine(module.entry_point) {}
};


static std::vector<library_init_routine> library_init_routines;
static std::unordered_map<std::string, std::unique_ptr<module>> module_table;
static std::unordered_map<uint32_t, module*> hinstance_table;

static module* load_pe_module(std::string module_name);

static uint32_t find_function_by_ordinal(module* module, uint32_t biased_ordinal)
{
    assert(module->export_directory != 0);
    auto export_table = g2hx<IMAGE_EXPORT_DIRECTORY>(module->export_directory);
    auto export_entries = g2hx<DWORD>(module->image_base + export_table->ExportAddressTableRVA);
    
    uint32_t ordinal = biased_ordinal - export_table->OrdinalBase;
    
    DWORD rva = export_entries[ordinal];
    int inside_edata = rva >= module->export_directory && rva < module->export_directory + module->export_directory_size;
    assert(!inside_edata); // elsze this is a forwarder RVA, which is not supported
    
    return module->image_base + rva;
}

static uint32_t find_function_by_name(module* module, const char* name)
{
    assert(module->export_directory != 0);
    auto export_table = g2hx<IMAGE_EXPORT_DIRECTORY>(module->export_directory);
    
    assert(export_table->NamePointerRVA != 0);
    
    int name_count = export_table->NamePointerCount;
    auto names_table = g2hx<DWORD>(module->image_base + export_table->NamePointerRVA);
    auto ordinal_table = g2hx<WORD>(module->image_base + export_table->OrdinalTableRVA);
    
    for (int i = 0; i < name_count; i++) {
        auto candidate_name = g2hx<char>(module->image_base + names_table[i]);
        if (strcmp(candidate_name, name) == 0) {
            return find_function_by_ordinal(module, export_table->OrdinalBase + ordinal_table[i]);
        }
    }
    
    return 0;
}

static int process_reloc(const module& module, uint32_t page_rva, uint16_t relocation, int32_t difference) {
    void* address = g2h((relocation & 0x0fff) + page_rva + module.image_base);
    int type = relocation >> 12;
    switch (type) {
        case IMAGE_REL_BASED_ABSOLUTE:
            // Actually a no-op reloc
            return 0;
        case IMAGE_REL_BASED_HIGHLOW:
            // enforce natural alignment
            if ((uintptr_t)address % 0x4 == 0)
                *(uint32_t*)address = *(uint32_t*)address + difference;
            else {
                uint8_t* ptr = static_cast<uint8_t*>(address);
                uint32_t value = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
                value += difference;
                ptr[0] = value & 0xff;
                ptr[1] = (value >> 8) & 0xff;
                ptr[2] = (value >> 16) & 0xff;
                ptr[3] = (value >> 24) & 0xff;
            }
            return 0;
        default:
            uw_log("unsupported relocation type: %d\n", type);
            abort();
    }
}

// load and link pe file from data. fills module structure
static int load_pe(const uint8_t* pe_file_data, module& mod)
{
    uw_log("loading %s file from %p...\n", mod.name.c_str(), pe_file_data);
    
    IMAGE_DOS_HEADER *header = (IMAGE_DOS_HEADER*) pe_file_data;
    assert(header->e_magic == IMAGE_DOS_SIGNATURE);
    DWORD* pe_signature_address = (DWORD*)(pe_file_data + header->e_lfanew);
    assert(*pe_signature_address == IMAGE_NT_SIGNATURE);
    IMAGE_FILE_HEADER* nt_header = (IMAGE_FILE_HEADER*)(pe_signature_address + 1);
    
    assert(nt_header->Machine == IMAGE_FILE_MACHINE_I386);
    
    
    uw_log("NumberOfSections = %d\n", nt_header->NumberOfSections);
    uw_log("SizeOfOptionalHeader = %d\n", nt_header->SizeOfOptionalHeader);
    
    
#define print_char(c) uw_log(#c " is %s\n", (nt_header->Characteristics & c) ? "set" : "not set");
    print_char(IMAGE_FILE_RELOCS_STRIPPED);
    print_char(IMAGE_FILE_EXECUTABLE_IMAGE);
    print_char(IMAGE_FILE_32BIT_MACHINE);
    print_char(IMAGE_FILE_SYSTEM);
    print_char(IMAGE_FILE_DLL);
    print_char(IMAGE_FILE_UP_SYSTEM_ONLY);
#undef print_char
    
    int fixed = (nt_header->Characteristics & IMAGE_FILE_RELOCS_STRIPPED) != 0;
    
    int dll = (nt_header->Characteristics & IMAGE_FILE_DLL) != 0;
    int exe = !dll;
    assert(dll != exe);
    
    assert(nt_header->SizeOfOptionalHeader != 0);
    
    IMAGE_OPTIONAL_HEADER *op_header = (IMAGE_OPTIONAL_HEADER*)(nt_header + 1);
    
    assert(op_header->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC);
    assert(op_header->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI || op_header->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
    
    uw_log("NumberOfRvaAndSizes = %d\n", op_header->NumberOfRvaAndSizes);
    uw_log("SizeOfImage = %08x\n", op_header->SizeOfImage);
    uw_log("ImageBase = %08x\n", op_header->ImageBase);
    uw_log("AddressOfEntryPoint = %08x\n", op_header->AddressOfEntryPoint);
    uw_log("SizeOfStackReserve = %08x\n", op_header->SizeOfStackReserve);
    uw_log("SizeOfStackCommit = %08x\n", op_header->SizeOfStackCommit);
    
    uint32_t image_base;
    if (fixed) {
        image_base = op_header->ImageBase;
        int r = uw_target_map_memory_fixed(image_base, op_header->SizeOfImage, UW_PROT_N); /* reserve the memory */
        assert(r != -1);
    } else {
        image_base = uw_target_map_memory_dynamic(op_header->SizeOfImage, UW_PROT_N); /* reserve the memory */
        assert(image_base != (uint32_t)-1);
    }

    mod.image_base = image_base;
    mod.code_base = image_base + op_header->BaseOfCode;
    
    IMAGE_SECTION_HEADER* sections = (IMAGE_SECTION_HEADER*)(((uint8_t*)op_header) + nt_header->SizeOfOptionalHeader);
    uw_log("Sections:\n");
    uw_log(" #  name     bgnaddr  endaddr  datasize virtsize dataptr  access\n");
    for (int i = 0; i < nt_header->NumberOfSections; i++) {
        char name_buffer[IMAGE_SIZEOF_SHORT_NAME + 2];
        name_buffer[0] = '\0';
        strncat(name_buffer, (const char*)sections[i].Name, IMAGE_SIZEOF_SHORT_NAME);
        uint32_t start = sections[i].VirtualAddress;
        uint32_t size = sections[i].SizeOfRawData;
        uint32_t raw_ptr = sections[i].PointerToRawData;
        uint32_t vsize = uwin::align_up(sections[i].Misc.VirtualSize, uw_host_page_size);
        uint32_t vend = start + vsize;
        int read = (sections[i].Characteristics & IMAGE_SCN_MEM_READ) != 0;
        int write = (sections[i].Characteristics & IMAGE_SCN_MEM_WRITE) != 0;
        int execute = (sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        uw_log("%02d. %8s %08x %08x %08x %08x %08x %c%c%c\n", i, name_buffer, start, vend, size, vsize, raw_ptr, read ? 'r' : '-', write ? 'w' : '-', execute ? 'x' : '-');
        
        int r = uw_target_map_memory_fixed(image_base + start, vsize, UW_PROT_RW);
        assert(r != -1);
        
        void* sect = g2h(image_base + start);
        memcpy(sect, pe_file_data + raw_ptr, size);
    }
    
     //TODO: implement relocations
    if (!fixed && op_header->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC) {
        uw_log("Applying relocations...\n");
        int32_t difference = image_base - op_header->ImageBase;
        
        auto reloc_table = g2hx<BASE_RELOC_BLOCK>(image_base + op_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
        auto reloc_table_end = reinterpret_cast<BASE_RELOC_BLOCK*>(((uint8_t*)reloc_table) + op_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
        
        while (reloc_table < reloc_table_end) {
            for (size_t i = 0; i < (reloc_table->BlockSize - 8) / 2; i++) {
                process_reloc(mod, reloc_table->PageRVA, reloc_table->RelocData[i], difference);
            }
            
            reloc_table = reinterpret_cast<BASE_RELOC_BLOCK*>(((uint8_t*)reloc_table) + reloc_table->BlockSize);
        }
    }
     
    if (op_header->AddressOfEntryPoint != 0) {
        mod.entry_point = image_base + op_header->AddressOfEntryPoint;
    }
    else {
        mod.entry_point = 0;
    }
    if (op_header->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT) {
        mod.export_directory = image_base + op_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        mod.export_directory_size = op_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    } else {
        mod.export_directory = 0;
        mod.export_directory_size = 0;
    }

    mod.stack_size = op_header->SizeOfStackReserve;
    
    // now look at import table and populate it
    
    uw_log("Linking imports...\n");
    
    if (op_header->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT) {
        uw_log("op_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = %d\n", op_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);
        auto pimp = g2hx<IMAGE_IMPORT_MODULE_DIRECTORY>(image_base + op_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        while(pimp->dwLookupTableRVA) { /* the table is null-terminated */
            auto name = g2hx<const char>(image_base + pimp->dwRVAModuleName);

            module* dep = load_pe_module(name);
            
            auto pilt = g2hx<DWORD>(image_base + pimp->dwLookupTableRVA);
            auto thunk = g2hx<DWORD>(image_base + pimp->dwThunkTableRVA);
            
            while (*pilt != 0) {
                DWORD ilt = *pilt;
                
                const char* dbg_name = "<unnamed>";
                
                if (ilt & 0x80000000) {
                    *thunk = find_function_by_ordinal(dep, ilt & (~0x80000000));
                } else {
                    auto hint_entry = g2hx<HINT_NAME_IMPORT_ENTRY>(image_base + ilt);
                    dbg_name = (char*)hint_entry->szName; 
                    *thunk = find_function_by_name(dep, (char*)hint_entry->szName);
                }
                
                //uw_log("link %40s -> %08lx\n", dbg_name, (unsigned long)*thunk);
                
                if (*thunk == 0)
                    uw_log("cannot link to %s\n", dbg_name);
                assert(*thunk != 0);
                
                pilt++;
                thunk++;
            }
            pimp++;
        }
    }
    
    // finalize sections protection
    for (int i = 0; i < nt_header->NumberOfSections; i++) {
        uint32_t start = sections[i].VirtualAddress;
        uint32_t vsize = uwin::align_up(sections[i].Misc.VirtualSize, uw_host_page_size);
        int read = (sections[i].Characteristics & IMAGE_SCN_MEM_READ) != 0;
        int write = (sections[i].Characteristics & IMAGE_SCN_MEM_WRITE) != 0;
        
        int prot = UW_PROT_N;
        if (read && write) prot = UW_PROT_RW;
        else if (read) prot = UW_PROT_R;
        else if (write) prot = UW_PROT_RW; // no write-only for you.
        
        int r = uw_set_memprotect(image_base + start, vsize, prot);
        assert(r != -1);
    }
    
    if (dll && mod.entry_point != 0) {
        uw_log("%s has DllMain\n", mod.name.c_str());
        //library_init_routine rout = {pmodule->image_base, pmodule->entry_point};
        library_init_routines.emplace_back(mod);
        //stbds_arrput(library_init_routines, rout);
        //g_array_append_val(library_init_routines, rout);
    }
    
    uw_log("loading of %s is done\n", mod.name.c_str());
    
    return 0;
}

static std::string module_path;
__thread uint32_t* win32_last_error = NULL;

static module* load_pe_module(std::string module_name)
{
    assert(!module_path.empty());
    
    // leave only basename
    {
        int i = module_name.size();
        while (i > 0 && module_name[i] != '\\' && module_name[i] != '/')
            i--;
        if (module_name[i] == '\\' || module_name[i] == '/')
            i++;
        module_name.erase(module_name.begin(), module_name.begin() + i);
    }

    // lower case
    std::transform(module_name.begin(), module_name.end(), module_name.begin(), [](unsigned char c){ return std::tolower(c); });
    
    uw_log("importing %s...\n", module_name.c_str());
    
    {
        auto r = module_table.find(module_name);
        if (r != module_table.end())
            return r->second.get();
    }
    
    auto mod = std::make_unique<module>(module_name); //uw_new0(module, 1);
    
    int res = -1;
    bool found_override = false;
    for (size_t i = 0; i < sizeof(ldr_overrides) / sizeof(*ldr_overrides); i++) {
        if (module_name == ldr_overrides[i].name) {
            uw_log("found override\n");
            res = load_pe(ldr_overrides[i].data, *mod);
            found_override = true;
        }
        //if (uw_ascii_strcasecmp(module_name.c_str(), ldr_overrides[i].name) == 0) {
        //}
    }

    if (!found_override) {
        std::string full_filename = module_path + "/" + module_name;

        uw_log("override not found, opening %s...\n", full_filename.c_str());

        FILE *f = fcaseopen(full_filename.c_str(), "rb");
        assert(f != NULL);

        int r = fseek(f, 0, SEEK_END);
        assert(r != -1);
        long size = ftell(f);
        assert(size != -1);
        r = fseek(f, 0, SEEK_SET);
        assert(r != -1);

        //auto data = uw_new(uint8_t, size);
        std::vector<uint8_t> data(size);

        int read = fread(data.data(), 1, size, f);
        assert(read == size);
        fclose(f);

        res = load_pe(data.data(), *mod);
    }

    if (res == 0) {
        auto pmodule = mod.get();
        module_table[module_name] = std::move(mod);
        hinstance_table[pmodule->image_base] = pmodule;
        return pmodule;
    } else {
        return NULL;
    }
}

/*static void destroy_module(module *module)
{
    uw_free(module->name);
    uw_free(module);
}*/

static module* exec_module;

void* ldr_load_executable_and_prepare_initial_thread(const char* exec_path, uw_target_process_data_t *process_data)
{
    char* module_path_c = uw_path_get_dirname(exec_path);
    module_path = module_path_c;
    uw_free(module_path_c);

    char* exec_name = uw_path_get_basename(exec_path);
    
    uw_file_set_host_directory(module_path.c_str());
    
    uw_log("module_path = %s; exec_name = %s\n", module_path.c_str(), exec_name);
    
    // load executable and it's dependencies
    exec_module = load_pe_module(exec_name);
    // load init
    module* init_dll = load_pe_module("init.dll");
    
    // wa don't want to call init's DllMain (because it isn't a DllMain at all)
    library_init_routines.erase(library_init_routines.end() - 1);
    
    // put information about DllMain's to be called
    
    int init_count = library_init_routines.size();
    uint32_t init_list = uw_target_map_memory_dynamic(
            uwin::align_up((init_count + 1) * sizeof(library_init_routine), uw_host_page_size), UW_PROT_RW);
    auto h_init_list = g2hx<library_init_routine>(init_list);
    for (int i = 0; i < library_init_routines.size(); i++)
        h_init_list[i] = library_init_routines[i];

    h_init_list[init_count] = library_init_routine(0, 0);
    
    uw_log("put %d init routines\n", init_count);
    
    uint32_t command_line = uw_target_map_memory_dynamic(uwin::align_up(strlen(exec_name) + 1, uw_host_page_size),
                                                         UW_PROT_RW);
    strcpy(g2hx<char>(command_line), exec_name);
    
    memset(process_data, 0, sizeof(uw_target_process_data_t));
    
    process_data->hmodule = exec_module->image_base;
    process_data->idt_base = 0; // to be allocated by uw_cpu_loop
    process_data->library_init_list = init_list;
    process_data->command_line = command_line;
    process_data->init_entry = init_dll->entry_point;
    process_data->process_id = 2; // to make sure nothing breaks =)

    assert(init_dll->entry_point != 0);
    
    void* initial_thread_param = uw_create_initial_thread(process_data, exec_module->entry_point, 0, exec_module->stack_size);

    uw_free(exec_name);
    
    return initial_thread_param;
}

static uint32_t return_string_to_buffer(char* buffer, int buffer_size, const std::string& str) {
    if (buffer_size == 0)
        return 0;
    buffer[0] = '\0';
    int l = str.length();
    strncat(buffer, str.c_str(), buffer_size - 1);
    if (l > buffer_size)
        return buffer_size;
    return l;
} 

uint32_t ldr_get_module_filename(uint32_t module_handle, char* buffer, int buffer_size) {
    auto res = hinstance_table.find(module_handle);
    //module *res = stbds_hmget(hinstance_table, module_handle);
    if (res == hinstance_table.end()) {
        //win32_err = ERROR_FILE_NOT_FOUND;
        return 0;
    }
    std::string& hmod_name = res->second->name;
    std::string p = std::string(UW_GUEST_PROG_PATH) + "\\" + hmod_name;
    uint32_t r = return_string_to_buffer(buffer, buffer_size, p);
    //win32_err = ERROR_SUCCESS;
    return r;
}
uint32_t ldr_get_module_handle(const char* module_name) {
    if (module_name == NULL)
        return exec_module->image_base;

    std::string lo(module_name);

    std::transform(lo.begin(), lo.end(), lo.begin(), [](unsigned char c){ return std::tolower(c); });

    auto res = module_table.find(lo);
    if (res == module_table.end()) {
        // try adding .dll
        lo += ".dll";
        res = module_table.find(lo);
        if (res == module_table.end()) {
            //win32_err = ERROR_FILE_NOT_FOUND;

            uw_log("ldr_get_module_handle(%s) -> 0\n", module_name);
            return 0;
        }
    }
    uw_log("ldr_get_module_handle(%s) -> %08x\n", module_name, res->second->image_base);
    //win32_err = ERROR_SUCCESS;
    return res->second->image_base;
}

uint32_t ldr_get_proc_address(uint32_t module_handle, const char* proc_name) {
    auto res = hinstance_table.find(module_handle);
    assert(res != hinstance_table.end());

    return find_function_by_name(res->second, proc_name);
}

char* ldr_beautify_address(uint32_t addr) {
    std::string closest_name = "<unknown>";
    uint32_t closest_base = 0;

    for (auto const& x : module_table)
    {
        module& module = *x.second; //module_table[i].value;
        if (addr > module.image_base) {
            if (addr - module.image_base < addr - closest_base) {
                closest_name = module.name;
                closest_base = module.image_base;
                // TODO: check image size
            }
        }
    }
    
    return uw_strdup_printf("%20s + 0x%08x", closest_name.c_str(), addr - closest_base);
}

void ldr_write_gdb_setup_script(int port, std::filesystem::path debug_path, FILE* f) {
    fprintf(f, "target remote localhost:%d\n", port);

    for (auto const& x : module_table)
    {
        module& module = *x.second;
        const std::string& name = x.first;

        std::string raw_name = name;
        raw_name.erase(raw_name.begin() + raw_name.find_last_of('.'), raw_name.end());
        //*rindex(raw_name.data(), '.') = '\0';
        auto path = debug_path / (raw_name + ".elf");


        if (std::filesystem::exists(path)) {
            fprintf(f, "add-symbol-file %s 0x%08x\n", path.c_str(), (unsigned)module.code_base);
        }
    }
}

void ldr_print_memory_map(void) {
    for (auto const& x : module_table)
    {
        module& module = *x.second;
        std::string const& name = x.first;
        
        uw_log("%20s: 0x%08x\n", name.c_str(), (unsigned)module.image_base);
    }
}

uint32_t ldr_load_library(const char* module_name) {
    return load_pe_module(module_name)->image_base;
}

uint32_t ldr_get_entry_point(uint32_t module_handle) {
    auto res = hinstance_table.find(module_handle);
    assert(res != hinstance_table.end());
    return res->second->entry_point;
}
