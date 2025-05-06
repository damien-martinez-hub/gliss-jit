#define PY_SSIZE_T_CLEAN
#include <stdio.h>
#include <Python.h>
#include <$(proc)/api.h>
#include <$(proc)/loader.h>
#include <$(proc)/debug.h>
#include <$(proc)/mem.h>

#define FUN(n, d) 	{#n, n, METH_VARARGS, d }

#define RETURN_PTR(x)  		return PyCapsule_New(x, NULL, NULL)
#define RETURN_TPTR(t, x)	return PyCapsule_New(x, #t, NULL)
#define RETURN_NONE			Py_RETURN_NONE
#define RETURN_BOOL(x)		{ if(x) Py_RETURN_TRUE; else Py_RETURN_TRUE; }
#define RETURN_INT(x)		return Py_BuildValue("i", x)
#define RETURN_UINT(x)		return Py_BuildValue("I", x)
#define RETURN_LONG(x)		return Py_BuildValue("L", x)
#define RETURN_ULONG(x)		return Py_BuildValue("K", x)
#define RETURN_FLOAT(x)		return Py_BuildValue("f", x)
#define RETURN_DOUBLE(x)	return Py_BuildValue("d", x)
#define RETURN_STR(x)		return Py_BuildValue("s", x)

#define PTR(t, x)		((t *)PyCapsule_GetPointer(x, NULL))
#define TPTR(t, x)		((t *)PyCapsule_GetPointer(x, #t))


/****** Platform functions ******/

static PyObject *
new_platform(PyObject *self, PyObject *args) {
	$(proc)_platform_t *pf = $(proc)_new_platform();
	if(pf == NULL)
		RETURN_NONE;
	else
		RETURN_TPTR($(proc)_platform_t, pf);
}

static PyObject *
unlock_platform(PyObject *self, PyObject *args) {
	PyObject *obj;
	if(!PyArg_ParseTuple(args, "O", &obj))
		return NULL;
	$(proc)_platform_t *pf = TPTR($(proc)_platform_t, obj);
	$(proc)_unlock_platform(pf);
	RETURN_NONE;
}


static PyObject *
load_platform(PyObject *self, PyObject *args) {
	PyObject *obj;
	const char *name;
	if(!PyArg_ParseTuple(args, "Os", &obj, &name))
		return NULL;
	int res = $(proc)_load_platform(TPTR($(proc)_platform_t, obj), name);
	RETURN_INT(res);
}

static PyObject *
get_memory(PyObject *self, PyObject *args) {
	PyObject *pf;
	int index;
	if(!PyArg_ParseTuple(args, "Oi", &pf, &index))
		return NULL;
	$(proc)_memory_t *mem = $(proc)_get_memory(TPTR($(proc)_platform_t, pf), index);
	if(mem == NULL)
		RETURN_NONE;
	else
		RETURN_TPTR($(proc)_memory_t, mem);
}

static PyObject *
load(PyObject *self, PyObject *args) {
	PyObject *pf, *loader;
	if(!PyArg_ParseTuple(args, "OO", &pf, &loader))
		return NULL;
	$(proc)_load(TPTR($(proc)_platform_t, pf), TPTR($(proc)_loader_t, loader));
	RETURN_NONE;
}

void $(proc)_load($(proc)_platform_t *platform, struct $(proc)_loader_t *loader);


/****** State functions ******/

static PyObject *
new_state(PyObject *self, PyObject *args) {
	PyObject *pf;
	if(!PyArg_ParseTuple(args, "O", &pf))
		return NULL;
	RETURN_TPTR($(proc)_state_t, $(proc)_new_state(TPTR($(proc)_platform_t, pf)));
}

static PyObject *
delete_state(PyObject *self, PyObject *args) {
	PyObject *state;
	if(!PyArg_ParseTuple(args, "O", &state))
		return NULL;
	$(proc)_delete_state(TPTR($(proc)_state_t, state));
	RETURN_NONE;
}

static PyObject *
copy_state(PyObject *self, PyObject *args) {
	PyObject *state;
	if(!PyArg_ParseTuple(args, "O", &state))
		return NULL;
	RETURN_TPTR($(proc)_state_t, $(proc)_copy_state(TPTR($(proc)_state_t, state)));
}

static PyObject *
fork_state(PyObject *self, PyObject *args) {
	PyObject *state;
	if(!PyArg_ParseTuple(args, "O", &state))
		return NULL;
	RETURN_TPTR($(proc)_state_t, $(proc)_fork_state(TPTR($(proc)_state_t, state)));
}

static PyObject *
get_state_platform(PyObject *self, PyObject *args) {
	PyObject *state;
	if(!PyArg_ParseTuple(args, "O", &state))
		return NULL;
	RETURN_TPTR($(proc)_platform_t, TPTR($(proc)_state_t, state)->platform);
}

static PyObject *
get_state_memory(PyObject *self, PyObject *args) {
	PyObject *state;
	if(!PyArg_ParseTuple(args, "O", &state))
		return NULL;
	RETURN_TPTR($(proc)_memory_t, TPTR($(proc)_state_t, state)->M);
}

static PyObject *
get_register(PyObject *self, PyObject *args) {
	PyObject *state;
	int bi, ri;
	if(!PyArg_ParseTuple(args, "Oii", &state, &bi, &ri))
		return NULL;
	register_bank_t *banks = $(proc)_get_registers();
	register_value_t v =
		$(proc)_get_register(TPTR($(proc)_state_t, state), banks[bi].id, ri);
	switch(banks[bi].type) {
	case RTYPE_NONE:
	case RTYPE_BITS:
	case RTYPE_ADDR:
		if(banks[bi].tsize <= 32)
			RETURN_UINT(v.iv);
		else
			return Py_BuildValue("K", v.lv);
	case RTYPE_INT:
		if(banks[bi].tsize <= 32)
			RETURN_INT(v.iv);
		else
			return Py_BuildValue("L", v.lv);
	case RTYPE_FLOAT:
		return Py_BuildValue("d", v.dv);
	default:
		return NULL;
	}
}

static PyObject *
set_register(PyObject *self, PyObject *args) {
	register_value_t v;
	PyObject *state, *value;
	int bi, ri;
	if(!PyArg_ParseTuple(args, "OiiO", &state, &bi, &ri, &value))
		return NULL;
	register_bank_t *banks = $(proc)_get_registers();
	switch(banks[bi].type) {
	case RTYPE_NONE:
	case RTYPE_BITS:
	case RTYPE_ADDR:
		if(banks[bi].size <= 32)
			v.iv = PyLong_AsUnsignedLong(value);
		else
			v.lv = PyLong_AsUnsignedLongLong(value);
		break;
	case RTYPE_INT:
		if(banks[bi].size <= 32)
			v.iv = PyLong_AsLong(value);
		else
			v.lv = PyLong_AsLongLong(value);
		break;
	case RTYPE_FLOAT:
		if(banks[bi].size <= 32)
			v.fv = PyFloat_AsDouble(value);
		else
			v.dv = PyFloat_AsDouble(value);
		break;
	default:
		break;
	}
	$(proc)_set_register(TPTR($(proc)_state_t, state), banks[bi].id, ri, v);
	RETURN_NONE;
}
	

/****** Loader functions ******/


static PyObject *
loader_open(PyObject *self, PyObject *args) {
	const char *name;
	if(!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	$(proc)_loader_t *loader = $(proc)_loader_open(name);
	if(loader == NULL)
		RETURN_NONE;
	else
		RETURN_TPTR($(proc)_loader_t, loader);
}

static PyObject *
loader_close(PyObject *self, PyObject *args) {
	PyObject *loader;
	if(!PyArg_ParseTuple(args, "O", &loader))
		return NULL;
	$(proc)_loader_close(TPTR($(proc)_loader_t, loader));
	RETURN_NONE;
}

static PyObject *
loader_load(PyObject *self, PyObject *args) {
	PyObject *loader, *pf;
	if(!PyArg_ParseTuple(args, "OO", &loader, &pf))
		return NULL;
	$(proc)_loader_load(
		TPTR($(proc)_loader_t, loader),
		TPTR($(proc)_platform_t, pf));
	RETURN_NONE;
}

static PyObject *
loader_start(PyObject *self, PyObject *args) {
	PyObject *loader;
	if(!PyArg_ParseTuple(args, "O", &loader))
		return NULL;
	RETURN_UINT($(proc)_loader_start(TPTR($(proc)_loader_t, loader)));
}


static PyObject *
loader_count_syms(PyObject *self, PyObject *args) {
	PyObject *loader;
	if(!PyArg_ParseTuple(args, "O", &loader))
		return NULL;
	RETURN_INT($(proc)_loader_count_syms(TPTR($(proc)_loader_t, loader)));
}


static PyObject *
loader_sym(PyObject *self, PyObject *args) {
	PyObject *loader;
	int i;
	if(!PyArg_ParseTuple(args, "Oi", &loader, &i))
		return NULL;
	$(proc)_loader_sym_t sym;
	$(proc)_loader_sym(TPTR($(proc)_loader_t, loader), i, &sym);
	return Py_BuildValue("siiii", sym.name, sym.value, sym.size, sym.sect, sym.type, sym.bind);
}

static PyObject *
loader_count_sects(PyObject *self, PyObject *args) {
	PyObject *loader;
	if(!PyArg_ParseTuple(args, "O", &loader))
		return NULL;
	RETURN_UINT($(proc)_loader_count_sects(TPTR($(proc)_loader_t, loader)));
}

static PyObject *
loader_sect(PyObject *self, PyObject *args) {
	PyObject *loader;
	int i;
	if(!PyArg_ParseTuple(args, "Oi", &loader, &i))
		return NULL;
	$(proc)_loader_sect_t sect;
	$(proc)_loader_sect(TPTR($(proc)_loader_t, loader), i, &sect);
	return Py_BuildValue("sIII", sect.name, sect.addr, sect.size, sect.type);
}


/****** Simulation ******/

static PyObject *
new_sim(PyObject *self, PyObject *args) {
	PyObject *state;
	int start, exit;
	if(!PyArg_ParseTuple(args, "Oii", &state, &start, &exit))
		return NULL;
	RETURN_TPTR($(proc)_sim_t, $(proc)_new_sim(TPTR($(proc)_state_t, state), start, exit));
}

static PyObject *
delete_sim(PyObject *self, PyObject *args) {
	PyObject *sim;
	if(!PyArg_ParseTuple(args, "O", &sim))
		return NULL;
	$(proc)_delete_sim(TPTR($(proc)_sim_t, sim));
	RETURN_NONE;
}

static PyObject *
next_addr(PyObject *self, PyObject *args) {
	PyObject *sim;
	if(!PyArg_ParseTuple(args, "O", &sim))
		return NULL;
	RETURN_UINT($(proc)_next_addr(TPTR($(proc)_sim_t, sim)));
}

static PyObject *
next_inst(PyObject *self, PyObject *args) {
	PyObject *sim;
	if(!PyArg_ParseTuple(args, "O", &sim))
		return NULL;
	RETURN_TPTR($(proc)_inst_t, $(proc)_next_inst(TPTR($(proc)_sim_t, sim)));
}

static PyObject *
step(PyObject *self, PyObject *args) {
	PyObject *sim;
	if(!PyArg_ParseTuple(args, "O", &sim))
		return NULL;
	$(proc)_step(TPTR($(proc)_sim_t, sim));
	RETURN_NONE;
}

static PyObject *
set_next_address(PyObject *self, PyObject *args) {
	PyObject *sim;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &sim, &addr))
		return NULL;
	$(proc)_set_next_address(TPTR($(proc)_sim_t, sim), addr);
	RETURN_NONE;
}

static PyObject *
set_exit_address(PyObject *self, PyObject *args) {
	PyObject *sim;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &sim, &addr))
		return NULL;
	$(proc)_set_exit_address(TPTR($(proc)_sim_t, sim), addr);
	RETURN_NONE;
}

static PyObject *
set_sim_ended(PyObject *self, PyObject *args) {
	PyObject *sim;
	if(!PyArg_ParseTuple(args, "O", &sim))
		return NULL;
	$(proc)_set_sim_ended(TPTR($(proc)_sim_t, sim));
	RETURN_NONE;
}

static PyObject *
is_sim_ended(PyObject *self, PyObject *args) {
	PyObject *sim;
	if(!PyArg_ParseTuple(args, "O", &sim))
		return NULL;
	RETURN_BOOL($(proc)_is_sim_ended(TPTR($(proc)_sim_t, sim)));
}

static PyObject *
get_sim_state(PyObject *self, PyObject *args) {
	PyObject *sim;
	if(!PyArg_ParseTuple(args, "O", &sim))
		return NULL;
	RETURN_TPTR($(proc)_state_t, TPTR($(proc)_sim_t, sim)->state);
}

static PyObject *
get_sim_decoder(PyObject *self, PyObject *args) {
	PyObject *sim;
	if(!PyArg_ParseTuple(args, "O", &sim))
		return NULL;
	RETURN_TPTR($(proc)_decoder_t, TPTR($(proc)_sim_t, sim)->decoder);
}


/****** Inst ******/

static PyObject *
free_inst(PyObject *self, PyObject *args) {
	PyObject *inst;
	if(!PyArg_ParseTuple(args, "O", &inst))
		return NULL;
	$(proc)_free_inst(TPTR($(proc)_inst_t, inst));
	RETURN_NONE;
}

static PyObject *
disasm(PyObject *self, PyObject *args) {
	PyObject *inst;
	if(!PyArg_ParseTuple(args, "O", &inst))
		return NULL;
	char buf[256];
	$(proc)_disasm(buf, TPTR($(proc)_inst_t, inst));
	return Py_BuildValue("s", buf);
}

static PyObject *
get_inst_size(PyObject *self, PyObject *args) {
	PyObject *inst;
	if(!PyArg_ParseTuple(args, "O", &inst))
		return NULL;
	RETURN_UINT($(proc)_get_inst_size(TPTR($(proc)_inst_t, inst)));
}

static PyObject *
get_inst_addr(PyObject *self, PyObject *args) {
	PyObject *inst;
	if(!PyArg_ParseTuple(args, "O", &inst))
		return NULL;
	RETURN_UINT(TPTR($(proc)_inst_t, inst)->addr);
}

static PyObject *
execute(PyObject *self, PyObject *args) {
	PyObject *state;
	PyObject *inst;
	if(!PyArg_ParseTuple(args, "OO", &state, &inst))
		return NULL;
	$(proc)_execute(TPTR($(proc)_state_t, inst), TPTR($(proc)_inst_t, inst));
	RETURN_NONE;
}


/****** Debug ******/

static PyObject *
count_register_banks(PyObject *self, PyObject *args) {
	register_bank_t *banks = $(proc)_get_registers();
	int i;
	for(i = 0; banks[i].id >= 0; i++);
	RETURN_UINT(i);
}

static PyObject *
get_register_bank(PyObject *self, PyObject *args) {
	int i;
	if(!PyArg_ParseTuple(args, "i", &i))
		return NULL;
	register_bank_t *banks = $(proc)_get_registers();
	return Py_BuildValue("ssiii",
		banks[i].name, banks[i].format,
		banks[i].size, banks[i].type, banks[i].tsize);
}



/****** memory ******/

static PyObject *
mem_read8(PyObject *self, PyObject *args) {
	PyObject *mem;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &mem, &addr))
		return NULL;
	RETURN_UINT($(proc)_mem_read8(TPTR($(proc)_memory_t, mem), addr));
}

static PyObject *
mem_read16(PyObject *self, PyObject *args) {
	PyObject *mem;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &mem, &addr))
		return NULL;
	RETURN_UINT($(proc)_mem_read16(TPTR($(proc)_memory_t, mem), addr));
}

static PyObject *
mem_read32(PyObject *self, PyObject *args) {
	PyObject *mem;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &mem, &addr))
		return NULL;
	RETURN_UINT($(proc)_mem_read32(TPTR($(proc)_memory_t, mem), addr));
}

static PyObject *
mem_read64(PyObject *self, PyObject *args) {
	PyObject *mem;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &mem, &addr))
		return NULL;
	RETURN_ULONG($(proc)_mem_read64(TPTR($(proc)_memory_t, mem), addr));
}

static PyObject *
mem_readf(PyObject *self, PyObject *args) {
	PyObject *mem;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &mem, &addr))
		return NULL;
	RETURN_FLOAT($(proc)_mem_readf(TPTR($(proc)_memory_t, mem), addr));
}

static PyObject *
mem_readd(PyObject *self, PyObject *args) {
	PyObject *mem;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &mem, &addr))
		return NULL;
	RETURN_DOUBLE($(proc)_mem_readd(TPTR($(proc)_memory_t, mem), addr));
}


/****** Decoder ******/

static PyObject *
get_mode_count(PyObject *self, PyObject *args) {
	int i;
	for(i = 0; $(proc)_modes[i].name != NULL; i++);
	RETURN_UINT(i);
}

static PyObject *
get_mode_name(PyObject *self, PyObject *args) {
	int i;
	if(!PyArg_ParseTuple(args, "i", &i))
		return NULL;
	RETURN_STR($(proc)_modes[i].name);
}

static PyObject *
decode_mode(PyObject *self, PyObject *args) {
	PyObject *decoder;
	int i;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OiI", &decoder, &i, &addr))
		return NULL;
	RETURN_TPTR($(proc)_inst_t, $(proc)_modes[i].decode(TPTR($(proc)_decoder_t, decoder), addr));
	RETURN_NONE;
}

static PyObject *
new_decoder(PyObject *self, PyObject *args) {
	PyObject *pf;
	if(!PyArg_ParseTuple(args, "O", &pf))
		return NULL;
	RETURN_TPTR($(proc)_decoder_t, $(proc)_new_decoder(TPTR($(proc)_platform_t, pf)));
}

static PyObject *
delete_decoder(PyObject *self, PyObject *args) {
	PyObject *decoder;
	if(!PyArg_ParseTuple(args, "O", &decoder))
		return NULL;
	$(proc)_delete_decoder(TPTR($(proc)_decoder_t, decoder));
	RETURN_NONE;
}

static PyObject *
decode(PyObject *self, PyObject *args) {
	PyObject *decoder;
	$(proc)_address_t addr;
	if(!PyArg_ParseTuple(args, "OI", &decoder, &addr))
		return NULL;
	RETURN_TPTR($(proc)_inst_t, $(proc)_decode(TPTR($(proc)_decoder_t, decoder), addr));
}


/****** Module declaration ******/

static PyMethodDef $(proc)_methods[] = {
	
	FUN(new_platform, "Create and return a platform."),
	FUN(unlock_platform, "(platform): unlock a platform."),
	FUN(load_platform, "(platform, path): load a program in the platform."),
	FUN(get_memory, "(platform, index): get a memory of the platform."
		"Main memory has index 0."),
	FUN(load, "(platform, loader). Load in the platform from the given loader."),
	
	FUN(new_state, "Create a new simulation state."),
	FUN(delete_state, "(state) Delete a simulation state."),
	FUN(copy_state, "(state) Copy state for state saving."),
	FUN(fork_state, "(state) Copy state for further simulation."),
	FUN(execute, "(state, instruction) Execute the given instruction."),
	FUN(get_register, "(state, bank index, register index)"
		" Get the value of a register."),
	FUN(set_register, "(state, bank index, register index, value)"
		" Set the value of a register."),
	FUN(get_state_platform, "(state) Get the state platform."),
	FUN(get_state_memory, "(state) Get the main memory of the state."),

	FUN(new_sim, "(state, start address, exit address) Create a new simulation."
		"Exit address may be None."),
	FUN(delete_sim, "(simulation) Delete a simulation."),
	FUN(next_addr, "(simulation) Get the next address to be executed."),
	FUN(next_inst, "(simulation) Get the next instruction."),
	FUN(step, "(simulation) Perform one step of simulation."),
	FUN(set_next_address, "(simulation, address) Change the address of "
		"the next instruction to be executed."),
	FUN(set_exit_address, "(simulation, address) Change the exit address."),
	FUN(set_sim_ended, "(simulation) Mark the simulation as ended."),
	FUN(is_sim_ended, "(simulation) Test if the simulation is ended."),
	FUN(get_sim_state, "(simulation) Get the state of the simulation."),
	FUN(get_sim_decoder, "(simulation) Get the decoder of the simulation."),
	
	FUN(loader_open, "(program path) Open a loader."),
	FUN(loader_close, "(loader) Close a loader."),
	FUN(loader_load, "(loader, platform) Load the loader content in a platform."),
	FUN(loader_start, "(loader) Get the start address from the loader."),
	FUN(loader_count_syms, "(loader) Get the count of symbols."),
	FUN(loader_sym, "(loader, symbol index) Get a symbol description."
		"Return a tuple (name, value, size, section, type, bind)."
		"Type may be 0 for none, 1 for data, 2 for code."
		"Bind may be 0 for none, 1 for local, 2 global and 3 for weak."),
	FUN(loader_count_sects, "(loader) Get the count of sections."),
	FUN(loader_sect, "(loader, section number) Get information about a section:"
		"(name, address, size, type) with type 1 - TEXT, 2 - DATA, 3 - BSS."),
	
	
	FUN(free_inst, "(instruction) Free an instruction."),
	FUN(disasm, "(instruction) Disassemble an instruction abd return the corresponding string."),
	FUN(get_inst_size, "(instruction) Get the size of an instruction in bytes."),
	FUN(get_inst_addr, "(instrucrtion) Get an instruction address."),
	
	FUN(count_register_banks, "Count the number of register banks."),
	FUN(get_register_bank, "(bank index) Get a register bank."),
	
	FUN(mem_read8, "(memory, address) Read a byte in memory."),
	FUN(mem_read16, "(memory, address) Read an half-word in memory."),
	FUN(mem_read32, "(memory, address) Read a word in memory.S"),
	FUN(mem_read64, "(memory, address) Read a long word in memory."),
	FUN(mem_readf, "(memory, address) Read a float in memory."),
	FUN(mem_readd, "(memory, address) Read a double in memory."),
	
	FUN(decode, "(decoder, address) Decoder an instruction at the given address."),
	FUN(new_decoder, "(platform) Create a new decoder."),
	FUN(delete_decoder, "(decoder) Delete a decoder."),
	FUN(get_mode_count, "Get the count of decoding modes."),
	FUN(get_mode_name, "(mode index) Get the name of a decoding mode."),
	FUN(decode_mode, "(mode index, decoder, index) "
		"Decode an address according to a decoding mode."),

	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef $(proc)_module = {
    PyModuleDef_HEAD_INIT,
    "$(proc)gliss",
    "$(PROC) emulation library",
    -1,
    $(proc)_methods
};

PyMODINIT_FUNC
PyInit_$(proc)gliss(void) {
    return PyModule_Create(&$(proc)_module);
}
