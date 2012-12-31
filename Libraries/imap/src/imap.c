#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include "imapInternals.h"

/************************************************************************************************
	imap
*************************************************************************************************/

imap inputMap;

static void imapExecuteEntries();
static void	imapProcessKeyStates(imap_queue_entry* queue, int numEntries);
static int 	imapProcessButtonInput(imap_compiled_keymap_entry* entry);
static int 	imapProcessAxisInput(imap_compiled_keymap_entry* entry, imap_callback_ext_data* extdata);
static int 	imapProcessAxisRelInput(imap_compiled_keymap_entry* entry);
static int 	imapProcessAxisAbsInput(imap_compiled_keymap_entry* entry);
static int 	imapProcessAxisCalibration(imap_compiled_keymap_entry* entry, int param);
static int 	imapProcessComboInput(imap_compiled_keymap_entry* entry, int execute);

static void imapCompileKeymap();
static void imapCompileKeyState(unsigned int iac);
static void imapCompileFuncAxisState(unsigned int functionCode);
static void imapCompileEntry(imap_keymap_entry* entry);
static void imapFreeCompiledKeymap();

static imap_key_state* imapFindKeyState(unsigned int iac);
static imap_func_axis_state* imapFindFuncAxisState(unsigned int functionCode);
static imap_compiled_keymap_entry* imapFindCompiledEntry(unsigned int functionCode, int keymapType, unsigned int idx);
static imap_calibration* imapFindCalibration(unsigned int iac, int create);

static void imapUpdateControlCallbacks();
static imapControlCallback imapFindControlCallback(unsigned int functionCode);

static int imap_queue_entry_compare(const void* q1, const void* q2);

static long find_cubic_roots(const float coeff[4], float x[3]);
static unsigned int get_millisecond_ticks();

/************************************************************************************************
	imapInit
*************************************************************************************************/

int imapInit()
{
	if(inputMap.inited) return IMAP_NO_ERR;

	inputMap.inited = 1;
	
	return IMAP_NO_ERR;
}

/************************************************************************************************
	imapKill
*************************************************************************************************/

void imapKill()
{	
	imap_function *func, *nextfunc;
	
	int i;
	
	if(!inputMap.inited ) return;
	inputMap.inited  =  0;

	func = inputMap.functionTable;
	while(func)
	{
		nextfunc = func->next;
		free(func);
		func = nextfunc;
	}
		
	for(i = 0; i < IMAP_MAX_KEYMAPS; i++)
	{
		if(inputMap.keymaps[i])
			imapKeymapFree(inputMap.keymaps[i]);
	}
	
	imapClearAxisCalibration();
	imapFreeCompiledKeymap();
	
	memset(&inputMap, 0x0, sizeof(inputMap));
}

/************************************************************************************************
	imapProcessInputs
*************************************************************************************************/

void imapProcessInputs(imap_queue_entry* queue, int numEntries)
{	
	// compile keymaps and/or update controlcallback function pointers if needed
	if(inputMap.compile) imapCompileKeymap();
	if(inputMap.updateControlCallbacks) imapUpdateControlCallbacks();
	
	// Update key states
	imapProcessKeyStates(queue, numEntries);
	
	// Execute any entries that need to be executed
	imapExecuteEntries();
}

/************************************************************************************************
	imapExecuteFunction
*************************************************************************************************/

unsigned int imapExecuteFunction(unsigned int functionCode, int param, unsigned int unused)
{
	imapControlCallback func = imapFindControlCallback(functionCode);

	if(func) return func(param, functionCode, 0);

	return 0;
}

/************************************************************************************************
	imapRegisterFunction
*************************************************************************************************/

void imapRegisterFunction(unsigned int functionCode, int isAxis)
{
	imap_function* head;
	imap_function* entry = (imap_function*)calloc(1, sizeof(imap_function));
	
	if(entry)
	{
		entry->functionCode = functionCode;
		entry->isAxis = isAxis;
		entry->removable = 0;	
		entry->callback = NULL;
	}
	
	head = inputMap.functionTable;
	if(head)
		head->prev = entry;
	
	entry->next = head;
	entry->prev = NULL;
	
	inputMap.functionTable = entry;
}

/************************************************************************************************
	imapInstallControlCallback
*************************************************************************************************/

void imapInstallControlCallback(unsigned int functionCode, imapControlCallback callback)
{
	imap_function* func = inputMap.functionTable;
	
	while(func)
	{
		if(func->functionCode == functionCode)
		{
			func->removable = 1;
			func->callback = callback;
		}
		
		func = func->next;
	}
	inputMap.updateControlCallbacks = 1;	
}


/************************************************************************************************
	imapInstallNonRemovableControlCallback
*************************************************************************************************/

void imapInstallNonRemovableControlCallback(unsigned int functionCode, imapControlCallback callback)
{
	imap_function* func = inputMap.functionTable;
	
	while(func)
	{
		if(func->functionCode == functionCode)
		{
			func->removable = 0;
			func->callback = callback;
		}
		
		func = func->next;
	}
	inputMap.updateControlCallbacks = 1;	
}

/************************************************************************************************
	imapRemoveControlCallbacks
*************************************************************************************************/

void imapRemoveControlCallbacks()
{
	imap_function* func = inputMap.functionTable;
	
	while(func)
	{
		if(func->removable)
			func->callback = NULL;	
		func = func->next;
	}
	inputMap.updateControlCallbacks = 1;
}

/************************************************************************************************
	imapInstallCallbacks
*************************************************************************************************/

void imapInstallCallbacks(imapReadCallback read, imapWriteCallback write, 
							imapIACInfoCallback iacInfo, imapFunctionInfoCallback functionInfo)
{
	inputMap.read = read;
	inputMap.write = write;
	inputMap.iacInfo = iacInfo;
	inputMap.functionInfo = functionInfo;
}

/************************************************************************************************
	imapButtonInputState
*************************************************************************************************/

void imapButtonInputState(int enable)
{
	// clear out all button keystates if disabling
	if(!enable)
	{
		imap_key_state* keyState = inputMap.keyState;
		
		while(keyState)
		{
			if(!keyState->iacInfo.isAxis)
			{
				keyState->count = 0;
				keyState->state = 0;
			}
			
			keyState = keyState->next;
		}
		
		// go ahead and un-execute any currently executing functions
		if(inputMap.compile) imapCompileKeymap(); // just to be paranoid
		if(inputMap.updateControlCallbacks) imapUpdateControlCallbacks();
			
		imapExecuteEntries();
	}
	
	inputMap.buttonInputEnabled = enable;
}

/************************************************************************************************
	imapAxisInputState
*************************************************************************************************/

void imapAxisInputState(int enable)
{
	inputMap.axisInputEnabled = enable;
}

/************************************************************************************************
	imapCenterAxis
*************************************************************************************************/

void imapCenterAxis(unsigned int iac)
{
	imap_compiled_keymap_entry* entry = inputMap.compiledKeymap;
	
	while(entry)
	{
		if(entry->funcAxisState && entry->keymap.keymapType == IMAP_AXIS_KEYMAP && entry->keymap.axis.iac == iac)
		{
			entry->param = 0;
			entry->funcAxisState->param = 0;
			entry->execute = 1;
		}
		entry = entry->next;
	}
}

/************************************************************************************************
	imapCalibrateAxis
*************************************************************************************************/

void imapCalibrateAxis(unsigned int iac, int scaleType, int numParams, float* params)
{
	imap_calibration* calib = imapFindCalibration(iac, 1);
	if(calib)
	{
		calib->params = (float*)realloc(calib->params, numParams * sizeof(float));
		
		if(calib->params)
		{		
			calib->iac = iac;
			calib->scaleType = scaleType;
			calib->numParams = numParams;
			memcpy(calib->params, params, numParams * sizeof(float));
			
			// create axis table if bezier
			if(scaleType == IMAP_CALIBRATE_BEZIER && calib->numParams == 4)
			{
				int x, numroots;
				float x0, x1, x2, x3;
				float y0, y1, y2, y3;
				float t, ta[3], coeff[4];
				int deadband, fullband;
				
				deadband = (int)(calib->params[0] * IMAP_MAX_VALUE);
				fullband = (int)(calib->params[3] * IMAP_MAX_VALUE);
	
				x0 = calib->params[0];
				x1 = x2 = calib->params[1];
				x3 = calib->params[3] ;
				
				y0 = 0.0f;
				y1 = y2 = calib->params[2];
				y3 = 1.0f;
				
				coeff[1] = 3 * (x1 - x0);
				coeff[2] = 3 * (x2 - x1) - coeff[1];
				coeff[3] = x3 - x0 - coeff[1] - coeff[2];
							
				for(x = 0; x < IMAP_MAX_VALUE + 1; x++)
				{	
					calib->axisMap[x] = x;
					
					if(x < deadband)
					{
						calib->axisMap[x] = 0;
					}else if(x > fullband)
					{
						calib->axisMap[x] = IMAP_MAX_VALUE;
					}else
					{
						coeff[0] = x0 - x / (float)IMAP_MAX_VALUE;												

						numroots = find_cubic_roots(coeff,ta);
						if(numroots != 1) continue;
						
						t = ta[0];
						if(t < 0 || t > 1) continue;

						calib->axisMap[x] = (int)(IMAP_MAX_VALUE * ( /*(1 - t) * (1 - t) * (1 - t) * y0 +*/
												3 * t * (1 - t) * (1 - t) * y1 +
	            								3 * t * t * (1 - t) * y2 +
	            								t * t * t * y3));	            							
	            	}
				}
			}
		}
	}
}

/************************************************************************************************
	imapClearAxisCalibration
*************************************************************************************************/

void imapClearAxisCalibration()
{		
	imap_calibration *calib, *nextcalib;

	calib = inputMap.calibration;
	while(calib)
	{
		nextcalib = calib->next;
		if(calib->params)
			free(calib->params);
		free(calib);
		calib = nextcalib;
	}
	
	inputMap.calibration = NULL;
}

/************************************************************************************************
	imapSetKeymap
*************************************************************************************************/

int imapSetKeymap(const imapKeymapRef ref, int idx)
{
	if(idx >= IMAP_MAX_KEYMAPS) return IMAP_MAX_KEYMAPS_ERR;
	
	if(inputMap.keymaps[idx] == NULL) inputMap.keymaps[idx] = imapKeymapAlloc();
	
	if(inputMap.keymaps[idx] == NULL) return IMAP_MEMORY_ERR;
	
	if(ref)
		imapKeymapCopy(inputMap.keymaps[idx], ref);
	else
		imapKeymapRemoveAll(inputMap.keymaps[idx]);
	
	inputMap.compile = 1;
	
	return IMAP_NO_ERR;
}

/************************************************************************************************
	imapDump
*************************************************************************************************/

void imapDump()
{
	imap_compiled_keymap_entry* keymap = inputMap.compiledKeymap;
	imap_iac_info iacInfo;
	imap_function_info funcInfo;
	char name[128];
	int i;
	
	printf("imapDump\n{\n");
	
	printf("\tcompiledKeymap\n\t{\n");
	while(keymap)
	{
		inputMap.functionInfo(IMAP_HAVE_CODE, "ENGLISH", &keymap->functionCode, name, 127, &funcInfo);

		printf( "\t\tfunctionCode: 0x%x, \"%s\", keymapType: %d, numKeys: %d\n", keymap->functionCode, 
						name, keymap->keymap.keymapType, keymap->keymap.numKeys);

		printf("\t\tkeyState\n\t\t{\n");
		for(i = 0; i < keymap->keymap.numKeys && i < IMAP_MAX_COMBO_KEYS; i++)
		{			
			inputMap.iacInfo(IMAP_HAVE_CODE, "English", "US", &keymap->keyState[i]->iac, name, 127, &iacInfo);
			printf( "\t\t\tiac: 0x%x, \"%s\"\n",keymap->keyState[i]->iac, name);
		}
		printf("\t\t}\n");
	
		keymap = keymap->next;
	}

	printf("\t}\n");
	printf( "}\n");
}

/************************************************************************************************
	imapExecuteEntries
*************************************************************************************************/

int imapGetKeymap(unsigned int functionCode, int index, const imap_keymap** keymap)
{
	imap_compiled_keymap_entry* entry;
	int i = 0;

	if(inputMap.compile) imapCompileKeymap();
	
	*keymap = NULL;
	
	entry = inputMap.compiledKeymap;
	
	for(; entry; entry = entry->next)
	{
		if(entry->functionCode == functionCode)
		{
			if(i == index)
			{
				 *keymap = &entry->keymap;
			 	return 0;
			}
			i++;
		}
	}
	
	return -1;
}

/************************************************************************************************
	imapExecuteEntries
*************************************************************************************************/

void imapExecuteEntries()
{
	imap_compiled_keymap_entry* entry;
	int execute;
	imap_callback_ext_data extdata;

// Loop through entries to see if we need to execute any
	for(entry = inputMap.compiledKeymap; entry; entry = entry->next)
	{
		if(entry->disabled) continue;
		
		execute = 0;
		memset(&extdata, 0x0, sizeof(extdata));
		
		if(1)
		{
			switch(entry->keymap.keymapType)
			{
				case IMAP_BUTTON_KEYMAP:
					execute = imapProcessButtonInput(entry);
				break;	
				case IMAP_AXIS_KEYMAP:
					extdata.axisType = entry->keyState[0]->iacInfo.axisType;
					extdata.emulateJoystick = entry->keymap.axis.emulateJoystick;
					execute = imapProcessAxisInput(entry, &extdata);
				break;
				case IMAP_AXIS_REL_KEYMAP:
					extdata.axisType = IMAP_SYMMETRIC_AXIS;
					execute = imapProcessAxisRelInput(entry);
				break;
				case IMAP_AXIS_ABS_KEYMAP:
					extdata.axisType = IMAP_SYMMETRIC_AXIS;
					execute = imapProcessAxisAbsInput(entry);			
				break;
			}
			
			execute = imapProcessComboInput(entry, execute);
			
			// clamp to -1000 to 1000
			if(entry->funcAxisState && entry->funcAxisState->param >  IMAP_MAX_VALUE) entry->funcAxisState->param =  IMAP_MAX_VALUE;
			if(entry->funcAxisState && entry->funcAxisState->param < -IMAP_MAX_VALUE) entry->funcAxisState->param = -IMAP_MAX_VALUE;
			if(entry->param >  IMAP_MAX_VALUE) entry->param =  IMAP_MAX_VALUE;
			if(entry->param < -IMAP_MAX_VALUE) entry->param = -IMAP_MAX_VALUE;
			
			execute |= entry->execute;
			entry->execute = 0;
			
			if(execute && entry->controlCallback)
				entry->controlCallback(entry->param, entry->functionCode, (unsigned int)&extdata);
		}	
	}
}

/************************************************************************************************
	imapProcessKeyStates
*************************************************************************************************/

static void imapProcessKeyStates(imap_queue_entry* queue, int numEntries)
{
	imap_key_state* keyState = inputMap.keyState;
	int i;
	
	qsort(queue, numEntries, sizeof(imap_queue_entry), imap_queue_entry_compare); // sort by iac code

	for(i = 0; i < numEntries; i++)
	{
		while(keyState)
		{	
			if(keyState->iac > queue[i].iac) break;
			
			keyState->gotinput = 0;
			keyState->emulateKeyUp = 0;	
			
			if(queue[i].iac == keyState->iac)
			{
				if( ( keyState->iacInfo.isAxis && inputMap.axisInputEnabled) ||
					(!keyState->iacInfo.isAxis && inputMap.buttonInputEnabled) )
				{
					keyState->emulateKeyUp = (keyState->state == 1 && queue[i].state == 1) ? 1 : 0;
					
					keyState->count = queue[i].count;
					keyState->state = queue[i].state;
					keyState->gotinput = 1;
				}
			}
			
			keyState = keyState->next;
		}
	}
	
	// clear out any left
	while(keyState)
	{
		keyState->gotinput = 0;
		keyState->emulateKeyUp = 0;
		keyState = keyState->next;
	}
}

/************************************************************************************************
	imapProcessButtonInput
*************************************************************************************************/

static int imapProcessButtonInput(imap_compiled_keymap_entry* entry)
{
	int i, execute;
	
	if(!inputMap.buttonInputEnabled) return 0;
	
	for(i = 0; i < entry->keymap.numKeys && i < IMAP_MAX_COMBO_KEYS; i++)
	{
		if(entry->keyState[i]->emulateKeyUp)
		{
			entry->executing = 0;
			break;
		}
	}
	
	execute = !entry->executing;

	for(i = 0; i < entry->keymap.numKeys && i < IMAP_MAX_COMBO_KEYS; i++)
	{
		if(entry->executing)
			execute = execute || !entry->keyState[i]->state;
		else
			execute = execute &&  entry->keyState[i]->state;
	}

	if(execute)
	{
		entry->executing = !entry->executing;
		entry->param = (entry->executing) ? 1 : 0;
	}
	
	return execute;
}

/************************************************************************************************
	imapProcessAxisInput
*************************************************************************************************/

static int imapProcessAxisInput(imap_compiled_keymap_entry* entry, imap_callback_ext_data* extdata)
{
	int param;

	if(!inputMap.axisInputEnabled || entry->keyState[0]->iacInfo.axisType  == 0 ||
		entry->keyState[0]->iac == IMAP_IAC_NOKEY || !entry->keyState[0]->gotinput) return 0;
				
	if(entry->keyState[0]->iacInfo.axisType != IMAP_DELTA_AXIS)
	{
		switch(entry->keymap.axis.range)
		{
			case 0:
				param = entry->keyState[0]->count;
			break;
			
			case 1:
				param = entry->keyState[0]->count * 2 - entry->keymap.axis.range * IMAP_MAX_VALUE;
			break;
			
			case -1:
				param =-entry->keyState[0]->count * 2 + entry->keymap.axis.range * IMAP_MAX_VALUE;
			break;
		}
	}else
	{
		param = entry->keyState[0]->count;
	}
								
	param = imapProcessAxisCalibration(entry, param);
		
	if(entry->keymap.axis.invert) param = -param;

	if(entry->keyState[0]->iacInfo.axisType == IMAP_DELTA_AXIS)
		extdata->deltaInput = param;
		
	// handle delta mouse input to non-delta axis
	if(entry->keyState[0]->iacInfo.axisType == IMAP_DELTA_AXIS && entry->funcInfo.axisType != IMAP_DELTA_AXIS) 
	{
		if(entry->keymap.axis.range < 0) param = -param;
		param = entry->param + param;
	}
	
	entry->funcAxisState->param = entry->param = param;
	entry->executing = 1;

	return 1;
}

/************************************************************************************************
	imapProcessAxisRelInput
*************************************************************************************************/

static int imapProcessAxisRelInput(imap_compiled_keymap_entry* entry)
{
	int i, execute;
	
	if(!inputMap.axisInputEnabled || !inputMap.buttonInputEnabled) return 0;
			
	execute = 1;

	for(i = 0; i < entry->keymap.numKeys && i < IMAP_MAX_COMBO_KEYS; i++)
	{
		execute = execute && entry->keyState[i]->state;
		if(entry->keyState[i]->emulateKeyUp) entry->executing = 0;
	}

	if(execute)
	{
		switch(entry->keymap.axisRel.axisMapType)
		{
			case IMAP_AXIS_PER_STROKE:
				if(!entry->executing)
				{
					entry->funcAxisState->param += (int)(entry->keymap.axisRel.rate * 2 * IMAP_MAX_VALUE);
					entry->executing = 1;
				}else
					execute = 0;
			break;
			
			case IMAP_AXIS_PER_SEC:
				if(!entry->executing)
					entry->lastExecuteTime = get_millisecond_ticks();
					
				entry->funcAxisState->param += (int)(entry->keymap.axisRel.rate * 2 * IMAP_MAX_VALUE * (get_millisecond_ticks() - entry->lastExecuteTime) / 1000.f);
				entry->executing = 1;
			break;
		}
								
		entry->param = entry->funcAxisState->param;
	}else
	{
		entry->executing = 0;
	}
	
	return execute;
}

/************************************************************************************************
	imapProcessAxisAbsInput
*************************************************************************************************/

static int imapProcessAxisAbsInput(imap_compiled_keymap_entry* entry)
{
	int i, execute;
	
	if(!inputMap.axisInputEnabled || !inputMap.buttonInputEnabled) return 0;			
	
	execute = 1;
		
	for(i = 0; i < entry->keymap.numKeys && i < IMAP_MAX_COMBO_KEYS; i++)
	{
		execute = execute && entry->keyState[i]->state;
		if(entry->keyState[i]->emulateKeyUp) entry->executing = 0;
	}
	
	if(execute)
	{
		entry->executing = 1;
		entry->funcAxisState->param = entry->param = (int)(2*IMAP_MAX_VALUE * entry->keymap.axisAbs.percent - IMAP_MAX_VALUE);
	}
	else 
	{ 				
		if(entry->keymap.axisAbs.onRelease && entry->executing)
		{
			execute = 1;
			entry->funcAxisState->param = entry->param = (int)(2*IMAP_MAX_VALUE * entry->keymap.axisAbs.releasePercent - IMAP_MAX_VALUE);
		}

		entry->executing = 0;
	}

	return execute;
}

/************************************************************************************************
	imapProcessAxisCalibration
*************************************************************************************************/

static int imapProcessAxisCalibration(imap_compiled_keymap_entry* entry, int param)
{
	imap_calibration* calib;
	
	if(entry->calibration == NULL) return param;

	calib = entry->calibration;
	
	switch(calib->scaleType)
	{
		case IMAP_CALIBRATE_LINEAR:
			if(calib->numParams == 1 && entry->keyState[0]->iacInfo.axisType == IMAP_DELTA_AXIS && calib->params[0] > 1.0)
			{	
				float dt, accel, speed = 0;
				float sens = (calib->params[0] - 1) / 150.0f; // arbritrary scale 1 -> 0, 4 -> 1/50
										
				if(entry->lastExecuteTime == 0) entry->lastExecuteTime = get_millisecond_ticks();
				
				dt = (get_millisecond_ticks() - entry->lastExecuteTime) / 1000.0f;
				if(dt) speed = param / dt;
				
				accel = (float)fabs(speed * sens);
				// clamp
				if(accel < 1.0f)  accel = 1.0f;
				if(accel > 10.0f) accel = 10.0f;
				
				param = (int)(param * accel); 
				
				entry->lastExecuteTime = get_millisecond_ticks();
			}
		break;
		
		case IMAP_CALIBRATE_BEZIER:
			if(calib->numParams == 4 && entry->keyState[0]->iacInfo.axisType != IMAP_DELTA_AXIS &&
				entry->funcInfo.axisType == IMAP_SYMMETRIC_AXIS)
			{
				if(param >= -1000 && param <= 1000)
				{
					int sign = (param > 0) ? 1 : -1;
					param = sign * calib->axisMap[param * sign];
				}
			}
		break;
	}
	
	return param;
}

/************************************************************************************************
	imapProcessComboInput
*************************************************************************************************/

int imapProcessComboInput(imap_compiled_keymap_entry* entry, int execute)
{
	int i;
	
	if(execute)
	{
		if(entry->keymap.numKeys == 1)
		{
			for(i = 0; i < entry->keymap.numKeys && i < IMAP_MAX_COMBO_KEYS; i++)
			{
				if(entry->keyState[i]->executingCombo)
				{
					execute = 0;
					break;
				}
			}
		}else
		{
			for(i = 0; i < entry->keymap.numKeys && i < IMAP_MAX_COMBO_KEYS; i++)
			{
				entry->keyState[i]->executingCombo = entry->executing;

				if(entry->keyState[i]->state && !entry->executing)
					entry->keyState[i]->emulateKeyUp = 1;
			}
		}
	}
		
	return execute;
}

/************************************************************************************************
	imapCompileKeymap
*************************************************************************************************/

static void imapCompileKeymap()
{
	int i,j;
	
	imapFreeCompiledKeymap();
	inputMap.compile = 0;
	inputMap.updateControlCallbacks = 0;
	
	for(i = 0; i < IMAP_MAX_KEYMAPS; i++)
	{
		if(inputMap.keymaps[i])
		{
			imap_keymap_entry* entry = inputMap.keymaps[i]->keymapEntries;
			
			while(entry)
			{
				if(entry->keymap.keymapType != IMAP_BUTTON_KEYMAP)
					imapCompileFuncAxisState(entry->functionCode);
					
				for(j = 0; j < entry->keymap.numKeys; j++)
				{
					switch(entry->keymap.keymapType)
					{
						case IMAP_BUTTON_KEYMAP:
							imapCompileKeyState(entry->keymap.btn.iac[j]);
						break;				
						case IMAP_AXIS_KEYMAP:
							imapCompileKeyState(entry->keymap.axis.iac);
						break;
						case IMAP_AXIS_REL_KEYMAP:
							imapCompileKeyState(entry->keymap.axisRel.iac[j]);
						break;
						case IMAP_AXIS_ABS_KEYMAP:
							imapCompileKeyState(entry->keymap.axisAbs.iac[j]);
						break;
					}
				}				
				imapCompileEntry(entry);
				entry = entry->next;
			}
		}
	}
}

/************************************************************************************************
	imapFreeCompiledKeymap
*************************************************************************************************/

static void imapFreeCompiledKeymap()
{
	imap_compiled_keymap_entry *keymap, *nextkeymap;
	imap_key_state *keystate, *nextkeystate;
	imap_func_axis_state *axisState, *nextAxisState;
	
	keymap = inputMap.compiledKeymap;
	while(keymap)
	{
		nextkeymap = keymap->next;
		free(keymap);
		keymap = nextkeymap;
	}
	inputMap.compiledKeymap = NULL;
	
	keystate = inputMap.keyState;
	while(keystate)
	{
		nextkeystate = keystate->next;
		free(keystate);
		keystate = nextkeystate;
	}
	inputMap.keyState = NULL;
	
	axisState = inputMap.funcAxisState;
	while(axisState)
	{
		nextAxisState = axisState->next;
		free(axisState);
		axisState = nextAxisState;
	}
	inputMap.funcAxisState = NULL;
}

/************************************************************************************************
	imapCompileKeyState
*************************************************************************************************/

static void imapCompileKeyState(unsigned int iac)
{
	imap_key_state* cur = inputMap.keyState;	
	imap_key_state* prev = NULL;
		
	do
	{		
		// already exists, no need to add
		if(cur && cur->iac == iac) break;

		// sorted insert
		if(cur == NULL || cur->iac > iac || cur->next == NULL)
		{
			imap_key_state* keyState = (imap_key_state*)calloc(1, sizeof(imap_key_state));
			keyState->iac = iac;
			inputMap.iacInfo(IMAP_HAVE_CODE, NULL, NULL, &iac, NULL, 0, &keyState->iacInfo);
			
			if(cur == NULL)
			{
				inputMap.keyState = keyState;
			}else if(cur->iac > iac)
			{
				if(prev)
					prev->next = keyState;
				else
					inputMap.keyState = keyState;
					
				keyState->next = cur;
			}else
			{
				cur->next = keyState;
			}
			
			break;
		}
		
		prev = cur;
		cur = cur->next;
	}while(cur);
}

/************************************************************************************************
	imapCompileFuncAxisState
*************************************************************************************************/

static void imapCompileFuncAxisState(unsigned int functionCode)
{
	imap_func_axis_state* funcAxisState = inputMap.funcAxisState;
	imap_func_axis_state* newState;
	imap_function_info funcInfo;
	
	while(funcAxisState)
	{
		if(funcAxisState->functionCode == functionCode) return;
		if(funcAxisState->next == NULL) break;
		
		funcAxisState = funcAxisState->next;
	}
	
	newState = (imap_func_axis_state*)calloc(1, sizeof(imap_func_axis_state));
	if(newState == NULL) return;
	
	newState->functionCode = functionCode;
	inputMap.functionInfo(IMAP_HAVE_CODE, NULL, &functionCode, NULL, 0, &funcInfo);

	if(funcInfo.axisType == IMAP_ASYMMETRIC_AXIS) 
		newState->param = -IMAP_MAX_VALUE;
	else
		newState->param = 0;
	
	if(funcAxisState)
		funcAxisState->next = newState;
	else
		inputMap.funcAxisState = newState;
}

/************************************************************************************************
	imapCompileEntry
*************************************************************************************************/

static void imapCompileEntry(imap_keymap_entry* entry)
{
	int i;
	imap_compiled_keymap_entry* compiledEntry;
	imapControlCallback func = imapFindControlCallback(entry->functionCode);
	
	if(entry->keymap.numKeys == 0) return;
	
	compiledEntry = (imap_compiled_keymap_entry*)calloc(1, sizeof(imap_compiled_keymap_entry));
	if(compiledEntry == NULL) return;
	
	compiledEntry->idx = entry->idx;
	compiledEntry->functionCode = entry->functionCode;
	compiledEntry->param = 0;
	compiledEntry->controlCallback = func;
	compiledEntry->keymap = entry->keymap;

	if(compiledEntry->keymap.keymapType != IMAP_BUTTON_KEYMAP)
		compiledEntry->funcAxisState = imapFindFuncAxisState(compiledEntry->functionCode);

	if(compiledEntry->keymap.keymapType == IMAP_AXIS_KEYMAP)
		compiledEntry->calibration = imapFindCalibration(compiledEntry->keymap.axis.iac, 0);
	
	inputMap.functionInfo(IMAP_HAVE_CODE, NULL, &entry->functionCode, NULL, 0, &compiledEntry->funcInfo);
	
	for(i = 0; i < entry->keymap.numKeys && i < IMAP_MAX_COMBO_KEYS; i++)
	{		
		switch(entry->keymap.keymapType)
		{
			case IMAP_BUTTON_KEYMAP:
				compiledEntry->keyState[i] = imapFindKeyState(entry->keymap.btn.iac[i]);
			break;
			case IMAP_AXIS_KEYMAP:
				compiledEntry->keyState[i] = imapFindKeyState(entry->keymap.axis.iac);
			break;
			case IMAP_AXIS_REL_KEYMAP:
				compiledEntry->keyState[i] = imapFindKeyState(entry->keymap.axisRel.iac[i]);
			break;
			case IMAP_AXIS_ABS_KEYMAP:
				compiledEntry->keyState[i] = imapFindKeyState(entry->keymap.axisAbs.iac[i]);
			break;
		}
		assert(compiledEntry->keyState[i]);
	}
	// Remove any previous entry for this functionCode.
	{
		imap_compiled_keymap_entry* oldEntry = imapFindCompiledEntry(entry->functionCode, entry->keymap.keymapType, entry->idx);
		if(oldEntry)
		{
			if(oldEntry->prev) 
			{
				oldEntry->prev->next = oldEntry->next;
			}else
			{
				inputMap.compiledKeymap = oldEntry->next;
			}
			
			if(oldEntry->next) oldEntry->next->prev = oldEntry->prev;
			
			free(oldEntry);
		}
	}

	// sorted insert, by numKeys
	{
		imap_compiled_keymap_entry* head = inputMap.compiledKeymap;
		imap_compiled_keymap_entry* prev = NULL;
		imap_compiled_keymap_entry* cur  = head;
		
		while(cur)
		{
			if(cur->keymap.numKeys <= compiledEntry->keymap.numKeys ) break;	
			prev = cur;
			cur = cur->next;
		}
		
		if(prev) prev->next = compiledEntry;
		else head = compiledEntry;
		
		if(cur) cur->prev = compiledEntry;
		
		compiledEntry->prev = prev;
		compiledEntry->next = cur;
		
		inputMap.compiledKeymap = head;
	}
	
	// non sorted insert
/*	compiledEntry->prev = NULL;
	compiledEntry->next = inputMap.compiledKeymap;
	if(compiledEntry->next) compiledEntry->next->prev = compiledEntry;
	inputMap.compiledKeymap = compiledEntry;
*/

}

/************************************************************************************************
	imapFindKeyState
*************************************************************************************************/

static imap_key_state* imapFindKeyState(unsigned int iac)
{
	imap_key_state* keyState = inputMap.keyState;
	
	while(keyState)
	{
		if(keyState->iac == iac) break;
		keyState = keyState->next;
	}
	
	return keyState;
}

/************************************************************************************************
	imapFindFuncAxisState
*************************************************************************************************/

static imap_func_axis_state* imapFindFuncAxisState(unsigned int functionCode)
{
	imap_func_axis_state* funcAxisState = inputMap.funcAxisState;
	
	while(funcAxisState)
	{
		if(funcAxisState->functionCode == functionCode) return funcAxisState;
		
		funcAxisState = funcAxisState->next;
	}
	
	return NULL;
}

/************************************************************************************************
	imapFindCompiledEntry
*************************************************************************************************/

static imap_compiled_keymap_entry* imapFindCompiledEntry(unsigned int functionCode, int keymapType, unsigned int idx)
{
	imap_compiled_keymap_entry* entry = inputMap.compiledKeymap;

	while(entry)
	{
		if(entry->functionCode == functionCode && entry->keymap.keymapType == keymapType && entry->idx == idx) return entry;
		entry = entry->next;
	}
	
	return NULL;
}

/************************************************************************************************
	imapFindCalibration
*************************************************************************************************/

static imap_calibration* imapFindCalibration(unsigned int iac, int create)
{
	imap_calibration* calib = inputMap.calibration;
	imap_calibration* newcalib;
	
	while(calib)
	{
		if(calib->iac == iac) return calib;
	
		if(calib->next == NULL) break;
		
		calib = calib->next;
	}
	
	if(!create) return NULL;
	
	newcalib = (imap_calibration*)calloc(1, sizeof(imap_calibration));
	if(newcalib == NULL) return NULL;
	
	newcalib->iac = iac;
	
	if(calib)
		calib->next = newcalib;
	else
		inputMap.calibration = newcalib;
	
	return newcalib;
}

/************************************************************************************************
	imapFindControlCallback
*************************************************************************************************/

static imapControlCallback imapFindControlCallback(unsigned int functionCode)
{
	imap_function* func = inputMap.functionTable;
	
	while(func)
	{
		if(func->functionCode == functionCode)
			return func->callback;
			
		func = func->next;
	}
	
	return NULL;
}

/************************************************************************************************
	imapUpdateControlCallbacks
*************************************************************************************************/

static void imapUpdateControlCallbacks()
{
	imap_compiled_keymap_entry* entry = inputMap.compiledKeymap;
	
	while(entry)
	{
		entry->controlCallback = imapFindControlCallback(entry->functionCode);
		entry = entry->next;
	}
	
	inputMap.updateControlCallbacks = 0;
}


/************************************************************************************************
	imapEnableFunction
*************************************************************************************************/

void imapEnableFunction(unsigned int functionCode, int enable)
{
	imap_compiled_keymap_entry* entry;
	
	for(entry = inputMap.compiledKeymap; entry; entry = entry->next)
	{
		if(entry->functionCode != functionCode) continue;
		
		entry->disabled = !enable;
	}
}

/************************************************************************************************
	imapEnableAllFunctions
*************************************************************************************************/

void imapEnableAllFunctions(int enableKeyFunctions, int enableAxisFunctions)
{
	imap_compiled_keymap_entry* entry;
	
	for(entry = inputMap.compiledKeymap; entry; entry = entry->next)
	{
		if(entry->keymap.keymapType == IMAP_BUTTON_KEYMAP)
		{
			entry->disabled = !enableKeyFunctions;
		}else
		{
			entry->disabled = !enableAxisFunctions;		
		}
	}
}

/************************************************************************************************
	imap_queue_entry_compare
*************************************************************************************************/

static int imap_queue_entry_compare(const void* v1, const void* v2)
{
	imap_queue_entry* q1 = (imap_queue_entry*)v1;
	imap_queue_entry* q2 = (imap_queue_entry*)v2;
	
	if(q1->iac > q2->iac) return 1;
	if(q1->iac < q2->iac) return -1;
	
	// In case we get duplicate iac's make sure key *up* gets put first, 
	// since we ignored all but the first
	if(q1->iac == q2->iac)
	{
		if(q1->state > q2->state) return 1;
		if(q1->state < q2->state) return -1;
	}
	
	return 0;
}

/************************************************************************************************
	find_cubic_roots
*************************************************************************************************/
// coeff[3] * x^3 + coeff[2] * x^2 + coeff[1] * x + coeff[0] = 0

#ifndef M_PI
#define	M_PI		3.14159265358979323846
#endif

static long find_cubic_roots(const float coeff[4], float x[3])
{
	float a1 = coeff[2] / coeff[3];
	float a2 = coeff[1] / coeff[3];
	float a3 = coeff[0] / coeff[3];

	double Q = (a1 * a1 - 3 * a2) / 9.0;
	double R = (2 * a1 * a1 * a1 - 9 * a1 * a2 + 27 * a3) / 54.0;
	double Qcubed = Q * Q * Q;
	double d = Qcubed - R * R;

	/* Three real roots */
	if (d >= 0) {
		double theta = acos(R / sqrt(Qcubed));
		double sqrtQ = sqrt(Q);
		x[0] = (float)(-2 * sqrtQ * cos( theta         	   / 3) - a1 / 3.0);
		x[1] = (float)(-2 * sqrtQ * cos((theta + 2 * M_PI) / 3) - a1 / 3.0);
		x[2] = (float)(-2 * sqrtQ * cos((theta + 4 * M_PI) / 3) - a1 / 3.0);
		return (3);
	}

	/* One real root */
	else {
		double e = pow(sqrt(-d) + fabs(R), 1.0 / 3.0);
		if (R > 0)
			e = -e;
		x[0] = (float)((e + Q / e) - a1 / 3.0);
		return (1);
	}
}

/************************************************************************************************
	get_millisecond_ticks
*************************************************************************************************/
#ifdef WIN32
#include <Windows.h>
#elif defined(__MACOS__)
#include <Timer.h>
#endif

static unsigned int get_millisecond_ticks()
{
	unsigned int ticks = 0;
#ifdef WIN32
	ticks = timeGetTime();
#elif defined(__APPLE__)
  uint64_t t = mach_absolute_time();
  static mach_timebase_info_data_t sTimebaseInfo;
  if ( sTimebaseInfo.denom == 0 ) 
  {
    mach_timebase_info(&sTimebaseInfo);
  }
  
  if ( sTimebaseInfo.denom != 1 || sTimebaseInfo.numer != 1 ) 
 	{
    ticks= (t * sTimebaseInfo.numer / sTimebaseInfo.denom)/1000/1000;
  }
  else 
  {
    ticks= t /1000/1000;
  }

#else
#error "no time func defined"
#endif
	return ticks;
}