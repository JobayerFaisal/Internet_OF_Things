//************************************************************
// This example uses the painlessMesh library for mesh communication
// 1. Sends a message to a specific node at a random time
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

// Define the target node ID (hardcoded for testing)
uint32_t targetNodeId = 1163312619;  // Change this to the target node's ID

// User stub
void sendMessage(); // Prototype for sendMessage to prevent PlatformIO complaints

// Task for sending messages at a random interval
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

// Function to send messages to a specific node
void sendMessage() {
  String msg = "Direct hello from node ";
  msg += mesh.getNodeId();  // Append node ID to the message

  // Check if the target node is connected before sending
  if (mesh.isConnected(targetNodeId)) {
    Serial.printf("Sending message: %s to node %u\n", msg.c_str(), targetNodeId);
    mesh.sendSingle(targetNodeId, msg);  // Send the message to the specific node
  } else {
    Serial.printf("Target node %u is not connected. Message not sent.\n", targetNodeId);
  }

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
