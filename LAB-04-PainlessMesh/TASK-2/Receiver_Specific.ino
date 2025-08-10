//************************************************************
// This example uses the painlessMesh library for mesh communication
// 1. Receives a message from another node and prints it
//
//************************************************************

#include "painlessMesh.h"

// Mesh parameters
#define MESH_PREFIX     "cse"
#define MESH_PASSWORD   "summer25"
#define MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// User stub
void receivedCallback(uint32_t from, String &msg);  // Prototype for receivedCallback

// Callback when a message is received
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

// Callback when a new node joins the mesh
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

// Callback when there is a change in the network topology
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

// Callback when the node's time is adjusted to mesh time
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);

  // Debug messages (optional, can be commented out for cleaner output)
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // You can add more types if needed

  // Initialize the mesh network
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  // Register callbacks for different mesh events
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
}

void loop() {
  // Update the mesh and run the scheduler
  mesh.update();
}
