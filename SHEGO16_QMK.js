export function Name() { return "SHEGO16"; }
export function Version() { return "1.1.9"; }
export function VendorId() { return 0xFADE; }
export function ProductId() { return 0x0666; }
export function Publisher() { return "Mars"; }
export function Documentation(){ return "qmk/srgbmods-qmk-firmware"; }
export function DeviceType() { return "keyboard"; }
export function Size() { return [1, 1]; }
export function DefaultPosition(){return [-100, -100]; }
export function DefaultScale(){return 8.0;}
/* global
shutdownMode:readonly
shutdownColor:readonly
LightingMode:readonly
forcedColor:readonly
*/
export function ControllableParameters() {
	return [
		{"property":"shutdownMode", "group":"lighting", "label":"Shutdown Mode", "type":"combobox", "values":["SignalRGB", "Hardware"], "default":"SignalRGB"},
		{"property":"shutdownColor", "group":"lighting", "label":"Shutdown Color", "min":"0", "max":"360", "type":"color", "default":"#000000"},
		{"property":"LightingMode", "group":"lighting", "label":"Lighting Mode", "type":"combobox", "values":["Canvas", "Forced"], "default":"Canvas"},
		{"property":"forcedColor", "group":"lighting", "label":"Forced Color", "min":"0", "max":"360", "type":"color", "default":"#009bde"},
	];
}

//Plugin Version: Built for Protocol V1.0.6

// Main keyboard keys (LEDs 0-14)
const vKeys = [
	2, 1, 0,           // Top row (no key for encoder knob)
	3, 4, 5, 6,        // Q, W, E, R row
	10, 9, 8, 7,       // A, S, D, F row
	11, 12, 13, 14     // Z, X, C, V row
];

// Ambient strip (LEDs 15-24)
const vAmbientKeys = [
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24
];

// Names for main keys
const vKeyNames = [
	"1", "2", "3",      // Top row
	"Q", "W", "E", "R", // Second row
	"A", "S", "D", "F", // Third row
	"Z", "X", "C", "V"  // Bottom row
];

// Names for ambient strip
const vAmbientKeyNames = [
	"Strip 1", "Strip 2", "Strip 3", "Strip 4", "Strip 5", 
	"Strip 6", "Strip 7", "Strip 8", "Strip 9", "Strip 10"
];

// Positions for main keys (4x4 grid area)
const vKeyPositions = [
	[0, 0], [1, 0], [2, 0],      // Top row (no [3,0])
	[0, 1], [1, 1], [2, 1], [3, 1],
	[0, 2], [1, 2], [2, 2], [3, 2],
	[0, 3], [1, 3], [2, 3], [3, 3]
];

// Positions for ambient strip (horizontal strip)
const vAmbientKeyPositions = [
	[0, 0], [1, 0], [2, 0], [3, 0], [4, 0], [5, 0], [6, 0], [7, 0], [8, 0], [9, 0]
];

let LEDCount = 25;
let IsViaKeyboard = false;
const MainlineQMKFirmware = 1;
const VIAFirmware = 2;
const PluginProtocolVersion = "1.0.6";

export function LedNames() {
	return [];
}

export function LedPositions() {
	return [];
}

export function LedGroups() {
	return [
		{ name: "Keys",    leds: vKeys },
		{ name: "Ambient Strip", leds: vAmbientKeys }
	];
}

export function vKeysArrayCount() {
	const totalKeys = vKeys.length + vAmbientKeys.length;
	const totalNames = vKeyNames.length + vAmbientKeyNames.length;
	const totalPositions = vKeyPositions.length + vAmbientKeyPositions.length;
	
	device.log('Main Keys: ' + vKeys.length);
	device.log('Ambient Keys: ' + vAmbientKeys.length);
	device.log('Total Keys: ' + totalKeys);
	device.log('Total Names: ' + totalNames);
	device.log('Total Positions: ' + totalPositions);
}

export function Initialize() {
	requestFirmwareType();
	requestQMKVersion();
	requestSignalRGBProtocolVersion();
	requestUniqueIdentifier();
	requestTotalLeds();
	
	// Create subdevices for independent zones
	device.createSubdevice("MainKeys");
	device.setSubdeviceName("MainKeys", "SHEGO Keys");
	device.setSubdeviceSize("MainKeys", 4, 4);
	device.setSubdeviceLeds("MainKeys", vKeyNames, vKeyPositions);

	device.createSubdevice("AmbientStrip");
	device.setSubdeviceName("AmbientStrip", "SHEGO Ambient Strip");
	device.setSubdeviceSize("AmbientStrip", 10, 1);
	device.setSubdeviceLeds("AmbientStrip", vAmbientKeyNames, vAmbientKeyPositions);
	
	effectEnable();
}

export function Render() {
	sendColors();
}

export function Shutdown(SystemSuspending) {
	if(SystemSuspending) {
		sendColors("#000000"); // Go Dark on System Sleep/Shutdown
	} else {
		if (shutdownMode === "SignalRGB") {
			sendColors(shutdownColor);
		} else {
			effectDisable();
		}
	}

	vKeysArrayCount(); // For debugging array counts
}

function commandHandler() {
	const readCounts = [];

	do {
		const returnpacket = device.read([0x00], 32, 10);
		processCommands(returnpacket);

		readCounts.push(device.getLastReadSize());

		// Extra Read to throw away empty packets from Via
		// Via always sends a second packet with the same Command Id.
		if(IsViaKeyboard) {
			device.read([0x00], 32, 10);
		}
	}
	while(device.getLastReadSize() > 0);
}

function processCommands(data) {
	switch(data[1]) {
	case 0x21:
		returnQMKVersion(data);
		break;
	case 0x22:
		returnSignalRGBProtocolVersion(data);
		break;
	case 0x23:
		returnUniqueIdentifier(data);
		break;
	case 0x24:
		sendColors();
		break;
	case 0x27:
		returnTotalLeds(data);
		break;
	case 0x28:
		returnFirmwareType(data);
		break;
	}
}

function requestQMKVersion() //Check the version of QMK Firmware that the keyboard is running
{
	device.write([0x00, 0x21], 32);
	device.pause(30);
	commandHandler();
}

function returnQMKVersion(data) {
	const QMKVersionByte1 = data[2];
	const QMKVersionByte2 = data[3];
	const QMKVersionByte3 = data[4];
	device.log("QMK Version: " + QMKVersionByte1 + "." + QMKVersionByte2 + "." + QMKVersionByte3);
	device.log("QMK SRGB Plugin Version: "+ Version());
	device.pause(30);
}

function requestSignalRGBProtocolVersion() //Grab the version of the SignalRGB Protocol the keyboard is running
{
	device.write([0x00, 0x22], 32);
	device.pause(30);
	commandHandler();
}

function returnSignalRGBProtocolVersion(data) {
	const ProtocolVersionByte1 = data[2];
	const ProtocolVersionByte2 = data[3];
	const ProtocolVersionByte3 = data[4];

	const SignalRGBProtocolVersion = ProtocolVersionByte1 + "." + ProtocolVersionByte2 + "." + ProtocolVersionByte3;
	device.log(`SignalRGB Protocol Version: ${SignalRGBProtocolVersion}`);

	if(PluginProtocolVersion !== SignalRGBProtocolVersion) {
		device.notify("Unsupported Protocol Version", `This plugin is intended for SignalRGB Protocol version ${PluginProtocolVersion}. This device is version: ${SignalRGBProtocolVersion}`, 2, "Documentation");
	}

	device.pause(30);
}

function requestUniqueIdentifier() //Grab the unique identifier for this keyboard model
{
	if(device.write([0x00, 0x23], 32) === -1) {
		device.notify("Unsupported Firmware", "This device is not running SignalRGB-compatible firmware. Click the Documentation button to learn more.", 3, "Documentation");
	}

	device.pause(30);
	commandHandler();
}

function returnUniqueIdentifier(data) {
	const UniqueIdentifierByte1 = data[2];
	const UniqueIdentifierByte2 = data[3];
	const UniqueIdentifierByte3 = data[4];

	if(!(UniqueIdentifierByte1 === 0 && UniqueIdentifierByte2 === 0 && UniqueIdentifierByte3 === 0)) {
		device.log("Unique Device Identifier: " + UniqueIdentifierByte1 + UniqueIdentifierByte2 + UniqueIdentifierByte3);
	}

	device.pause(30);
}

function requestTotalLeds() //Calculate total number of LEDs
{
	device.write([0x00, 0x27], 32);
	device.pause(30);
	commandHandler();
}

function returnTotalLeds(data) {
	LEDCount = data[2];
	device.log("Device Total LED Count: " + LEDCount);
	device.pause(30);
}

function requestFirmwareType() {
	device.write([0x00, 0x28], 32);
	device.pause(30);
	commandHandler();
}

function returnFirmwareType(data) {
	const FirmwareTypeByte = data[2];

	if(!(FirmwareTypeByte === MainlineQMKFirmware || FirmwareTypeByte === VIAFirmware)) {
		device.notify("Unsupported Firmware", "Click the Documentation button to learn more.", 3, "Documentation");
	}

	if(FirmwareTypeByte === MainlineQMKFirmware) {
		IsViaKeyboard = false;
		device.log("Firmware Type: Mainline");
	}

	if(FirmwareTypeByte === VIAFirmware) {
		IsViaKeyboard = true;
		device.log("Firmware Type: VIA");
	}

	device.pause(30);
}

function effectEnable() //Enable the SignalRGB Effect Mode
{
	device.write([0x00, 0x25], 32);
	device.pause(30);
}

function effectDisable() //Revert to Hardware Mode
{
	device.write([0x00, 0x26], 32);
	device.pause(30);
}

function createSolidColorArray(color) {
	const allKeys = vKeys.concat(vAmbientKeys);
	const rgbdata = new Array(allKeys.length * 3).fill(0);

	for(let iIdx = 0; iIdx < allKeys.length; iIdx++) {
		const iLedIdx = allKeys[iIdx] * 3;
		rgbdata[iLedIdx] = color[0];
		rgbdata[iLedIdx+1] = color[1];
		rgbdata[iLedIdx+2] = color[2];
	}

	return rgbdata;
}

function grabColors(overrideColor) {
	if(overrideColor) {
		return createSolidColorArray(hexToRgb(overrideColor));
	} else if (LightingMode === "Forced") {
		return createSolidColorArray(hexToRgb(forcedColor));
	}

	const rgbdata = new Array(25 * 3).fill(0);

	// Handle main keys (using MainKeys subdevice)
	for(let iIdx = 0; iIdx < vKeys.length; iIdx++) {
		const iPxX = vKeyPositions[iIdx][0];
		const iPxY = vKeyPositions[iIdx][1];
		const color = device.subdeviceColor("MainKeys", iPxX, iPxY);

		const iLedIdx = vKeys[iIdx] * 3;
		rgbdata[iLedIdx] = color[0];
		rgbdata[iLedIdx+1] = color[1];
		rgbdata[iLedIdx+2] = color[2];
	}

	// Handle ambient strip (using AmbientStrip subdevice)
	for(let iIdx = 0; iIdx < vAmbientKeys.length; iIdx++) {
		const iPxX = vAmbientKeyPositions[iIdx][0];
		const iPxY = vAmbientKeyPositions[iIdx][1];
		const color = device.subdeviceColor("AmbientStrip", iPxX, iPxY);

		const iLedIdx = vAmbientKeys[iIdx] * 3;
		rgbdata[iLedIdx] = color[0];
		rgbdata[iLedIdx+1] = color[1];
		rgbdata[iLedIdx+2] = color[2];
	}

	return rgbdata;
}

function sendColors(overrideColor) {
	const rgbdata = grabColors(overrideColor);

	const LedsPerPacket = 9;
	let BytesSent = 0;
	let BytesLeft = rgbdata.length;

	while(BytesLeft > 0) {
		const BytesToSend = Math.min(LedsPerPacket * 3, BytesLeft);
		StreamLightingData(Math.floor(BytesSent / 3), rgbdata.splice(0, BytesToSend));

		BytesLeft -= BytesToSend;
		BytesSent += BytesToSend;
	}
}

function StreamLightingData(StartLedIdx, RGBData) {
	const packet = [0x00, 0x24, StartLedIdx, Math.floor(RGBData.length / 3)].concat(RGBData);
	device.write(packet, 33);
}

function hexToRgb(hex) {
	const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
	const colors = [];
	colors[0] = parseInt(result[1], 16);
	colors[1] = parseInt(result[2], 16);
	colors[2] = parseInt(result[3], 2);

	return colors;
}

export function Validate(endpoint) {
	return endpoint.interface === 1;
}

export function Image() {
	return "";
}