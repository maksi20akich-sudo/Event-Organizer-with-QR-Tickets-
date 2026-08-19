🎟️ Event Organizer with QR Tickets — 
8 languages, one powerful event organizer – create events, register attendees, generate QR‑coded tickets, and manage check‑ins with ease.

✨ Features
📅 Create events with name, date, venue, and a unique ID

👤 Add attendees with name and email – each gets a unique ticket ID

📱 Generate QR tickets – each ticket contains a link to validate attendance

✅ Check‑in – mark attendees as present by scanning or entering ticket ID

📊 Dashboard – view event statistics and attendee lists

💾 Persistent storage – all data saved in a local JSON file

🌐 No external API needed – QR codes are generated locally using open‑source libraries

🖼️ PNG output – each ticket saved as a high‑quality image

🧰 Supported Languages
Language	File	QR Library
Python	event_organizer.py	qrcode + Pillow
Go	event_organizer.go	github.com/skip2/go-qrcode
JavaScript (Node)	event_organizer.js	qrcode + canvas
Ruby	event_organizer.rb	rqrcode + chunky_png
PHP	event_organizer.php	phpqrcode (or endroid/qr-code)
Java	EventOrganizer.java	com.google.zxing
C#	EventOrganizer.cs	QRCoder
C++	event_organizer.cpp	libqrencode
🚀 Quick Start
All implementations share the same CLI interface:

bash
# Create a new event
<program> event create "Tech Conference 2026" "2026-09-15" "Convention Center"

# List all events
<program> event list

# Add an attendee to an event (use event ID from list)
<program> attendee add <event_id> "John Doe" "john@example.com"

# Generate QR tickets for all attendees of an event
<program> ticket generate <event_id> -o ./tickets

# Check-in an attendee (mark as attended)
<program> checkin <event_id> <ticket_id>

# Show event status (attendee list with check-in status)
<program> status <event_id>
Commands & Options:

event create <name> <date> <venue> – create event

event list – show all events with IDs

attendee add <event_id> <name> <email> – add attendee

attendee list <event_id> – list attendees with ticket IDs

ticket generate <event_id> [-o dir] – generate QR tickets (default: current dir)

checkin <event_id> <ticket_id> – mark attendee as checked in

status <event_id> – show attendee list and check‑in stats

📁 Repository Structure
text
.
├── README.md
├── python/
│   └── event_organizer.py
├── go/
│   └── event_organizer.go
├── javascript/
│   └── event_organizer.js
├── ruby/
│   └── event_organizer.rb
├── php/
│   └── event_organizer.php
├── java/
│   └── EventOrganizer.java
├── csharp/
│   └── EventOrganizer.cs
└── cpp/
    └── event_organizer.cpp
