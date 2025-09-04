You can change these settings without being a programmer.

Open settings.txt and edit the words after the colon. This single file holds ALL settings, including enterprise Wi-Fi credentials.

Profiles (optional)
- You can keep multiple Wi-Fi configurations and switch quickly:
  Active Profile: office
  [profile: office]
  Wi-Fi Name: CorpSSID
  Use WPA2 Enterprise: yes
  Enterprise Username: your.name
  Enterprise Password: yourStrongPassword
  [profile: home]
  Wi-Fi Name: MyHome
  Wi-Fi Password: MySecret123

- Change Active Profile to pick which block applies. Lines outside any [profile: ...] apply to all profiles (e.g., LED Brightness).

Examples:
- Wi-Fi Name: My Home WiFi
- Wi-Fi Password: MySecret123
- Web Address Name: printer-cam
- LED Brightness (0-255): 40
- Picture Speed (seconds): 2

Enterprise Wi-Fi (username + password)
- Put these lines directly into settings.txt (no separate secrets.txt):
  Use WPA2 Enterprise: yes
  Enterprise Username: your.name
  Enterprise Password: yourStrongPassword
  Outer Identity (optional): anonymous
  CA Cert Path (optional): /you-can-change-this/ca.pem

Notes for Enterprise:
- If Use WPA2 Enterprise is "yes", the regular Wi-Fi Password is ignored.
- For best security, add your organization’s RADIUS CA certificate at the path above.

Notes:
- Don’t delete the part before the colon. Only change what’s after it.
- Keep numbers simple (like 2 or 10). The device understands seconds.
- If something breaks, just put it back to how it was and restart the device.
