// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "OThreadCLI.h"
#if SOC_IEEE802154_SUPPORTED
#if CONFIG_OPENTHREAD_ENABLED

#include "OThreadCLI_Util.h"
#include <StreamString.h>

bool otGetRespCmd(const char *cmd, char *resp, uint32_t respTimeout) {
  if (!OThreadCLI) {
    return false;
  }
  StreamString cliRespAllLines;
  char cliResp[256] = {0};
  if (resp != NULL) {
    *resp = '\0';
  }
  if (cmd == NULL) {
    return true;
  }
  OThreadCLI.println(cmd);
  log_d("CMD[%s]", cmd);
  uint32_t timeout = millis() + respTimeout;
  while (millis() < timeout) {
    size_t len = OThreadCLI.readBytesUntil('\n', cliResp, sizeof(cliResp));
    // clip it on EOL
    for (int i = 0; i < len; i++) {
      if (cliResp[i] == '\r' || cliResp[i] == '\n') {
        cliResp[i] = '\0';
      }
    }
    log_d("Resp[%s]", cliResp);
    if (strncmp(cliResp, "Done", 4) && strncmp(cliResp, "Error", 4)) {
      cliRespAllLines += cliResp;
      cliRespAllLines.println();  // Adds whatever EOL is for the OS
    } else {
      break;
    }
  }
  if (!strncmp(cliResp, "Error", 4) || millis() > timeout) {
    return false;
  }
  if (resp != NULL) {
    strcpy(resp, cliRespAllLines.c_str());
  }
  return true;
}

bool otExecCommand(const char *cmd, const char *arg, ot_cmd_return_t *returnCode) {
  if (!OThreadCLI) {
    return false;
  }
  char cliResp[256] = {0};
  if (cmd == NULL) {
    return true;
  }
  if (arg == NULL) {
    OThreadCLI.println(cmd);
  } else {
    OThreadCLI.print(cmd);
    OThreadCLI.print(" ");
    OThreadCLI.println(arg);
  }
  size_t len = OThreadCLI.readBytesUntil('\n', cliResp, sizeof(cliResp));
  // clip it on EOL
  for (int i = 0; i < len; i++) {
    if (cliResp[i] == '\r' || cliResp[i] == '\n') {
      cliResp[i] = '\0';
    }
  }
  log_d("CMD[%s %s] Resp[%s]", cmd, arg, cliResp);
  // initial returnCode is success values
  if (returnCode) {
    returnCode->errorCode = 0;
    returnCode->errorMessage = "Done";
  }
  if (!strncmp(cliResp, "Done", 4)) {
    return true;
  } else {
    if (returnCode) {
      // initial setting is a bad error message or it is something else...
      // return -1 and the full returned message
      returnCode->errorCode = -1;
      returnCode->errorMessage = cliResp;
      // parse cliResp looking for errorCode and errorMessage
      // OT CLI error message format is "Error ##: msg\n" - Example:
      //Error 35: InvalidCommand
      //Error 7: InvalidArgs
      char *i = cliResp;
      char *m = cliResp;
      while (*i && *i != ':') {
        i++;
      }
      if (*i) {
        *i = '\0';
        m = i + 2;  // message is 2 characters after ':'
        while (i > cliResp && *i != ' ') {
          i--;  // search for ' ' before ":'
        }
        if (*i == ' ') {
          i++;  // move it forward to the number beginning
          returnCode->errorCode = atoi(i);
          returnCode->errorMessage = m;
        }  // otherwise, it will keep the "bad error message" information
      }  // otherwise, it will keep the "bad error message" information
    }  // returnCode is NULL pointer
    return false;
  }
}

bool otPrintRespCLI(const char *cmd, Stream &output, uint32_t respTimeout) {
  char cliResp[256] = {0};
  if (cmd == NULL) {
    return true;
  }
  OThreadCLI.println(cmd);
  uint32_t timeout = millis() + respTimeout;
  while (millis() < timeout) {
    size_t len = OThreadCLI.readBytesUntil('\n', cliResp, sizeof(cliResp));
    if (cliResp[0] == '\0') {
      // Straem has timed out and it should try again using parameter respTimeout
      continue;
    }
    // clip it on EOL
    for (int i = 0; i < len; i++) {
      if (cliResp[i] == '\r' || cliResp[i] == '\n') {
        cliResp[i] = '\0';
      }
    }
    if (strncmp(cliResp, "Done", 4) && strncmp(cliResp, "Error", 4)) {
      output.println(cliResp);
      memset(cliResp, 0, sizeof(cliResp));
      timeout = millis() + respTimeout;  // renew timeout, line per line
    } else {
      break;
    }
  }
  if (!strncmp(cliResp, "Error", 4) || millis() > timeout) {
    return false;
  }
  return true;
}

void otCLIPrintNetworkInformation(Stream &output) {
  if (!OThreadCLI) {
    return;
  }
  char resp[512];
  output.println("Thread Setup:");
  if (otGetRespCmd("state", resp)) {
    output.printf("Node State: \t%s", resp);
  }
  if (otGetRespCmd("networkname", resp)) {
    output.printf("Network Name: \t%s", resp);
  }
  if (otGetRespCmd("channel", resp)) {
    output.printf("Channel: \t%s", resp);
  }
  if (otGetRespCmd("panid", resp)) {
    output.printf("Pan ID: \t%s", resp);
  }
  if (otGetRespCmd("extpanid", resp)) {
    output.printf("Ext Pan ID: \t%s", resp);
  }
  if (otGetRespCmd("networkkey", resp)) {
    output.printf("Network Key: \t%s", resp);
  }
  if (otGetRespCmd("ipaddr", resp)) {
    output.println("Node IP Addresses are:");
    output.printf("%s", resp);
  }
  if (otGetRespCmd("ipmaddr", resp)) {
    output.println("Node Multicast Addresses are:");
    output.printf("%s", resp);
  }
}

#endif /* CONFIG_OPENTHREAD_ENABLED */
#endif /* SOC_IEEE802154_SUPPORTED */
