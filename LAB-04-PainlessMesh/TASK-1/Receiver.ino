//************************************************************
// This example uses the painlessMesh library for mesh communication
// 1. Sends a message to every node on the mesh at a random time
// 2. Prints anything it receives to Serial.print
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
void sendMessage(); // Prototype for sendMessage to prevent PlatformIO complaints

// Task for sending messages at a random interval
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

// Function to send messages to nodes in the mesh
void sendMessage() {
  String msg = "Hi from Jobayer, NodeID: ";
  msg += mesh.getNodeId();  // Append node ID to the message
  Serial.printf("Sending message: %s\n", msg.c_str());
  
  // Send the message to specific nodes (update node IDs as required)
  mesh.sendSingle(1163312619, msg);  // Change node ID as per your network
  mesh.sendSingle(1163269966, msg);  // Change node ID as per your network
  
  // Set a random interval between 1 to 5 seconds for the next message
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

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

  // Add the task to the scheduler
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  // Update the mesh and run the scheduler
  mesh.update();
}
