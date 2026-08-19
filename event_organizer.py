# event_organizer.py
import sys, os, json, uuid, argparse, datetime, qrcode
from PIL import Image, ImageDraw, ImageFont

DATA_FILE = "events.json"

class Event:
    def __init__(self, name, date, venue):
        self.id = str(uuid.uuid4())[:8]
        self.name = name
        self.date = date
        self.venue = venue
        self.attendees = []  # list of dicts with id, name, email, checked_in

    def to_dict(self):
        return {
            "id": self.id,
            "name": self.name,
            "date": self.date,
            "venue": self.venue,
            "attendees": self.attendees
        }

    @classmethod
    def from_dict(cls, data):
        e = cls(data["name"], data["date"], data["venue"])
        e.id = data["id"]
        e.attendees = data["attendees"]
        return e

class Organizer:
    def __init__(self):
        self.events = []
        self.load()

    def load(self):
        if os.path.exists(DATA_FILE):
            with open(DATA_FILE, "r") as f:
                data = json.load(f)
                self.events = [Event.from_dict(e) for e in data]

    def save(self):
        with open(DATA_FILE, "w") as f:
            json.dump([e.to_dict() for e in self.events], f, indent=2)

    def create_event(self, name, date, venue):
        ev = Event(name, date, venue)
        self.events.append(ev)
        self.save()
        print(f"Event created: {ev.id} - {name}")
        return ev

    def list_events(self):
        if not self.events:
            print("No events.")
            return
        print("\n📅 Events:")
        for e in self.events:
            print(f"  {e.id}: {e.name} ({e.date}) at {e.venue} - {len(e.attendees)} attendees")

    def add_attendee(self, event_id, name, email):
        ev = next((e for e in self.events if e.id == event_id), None)
        if not ev:
            print(f"Event {event_id} not found.")
            return
        ticket_id = str(uuid.uuid4())[:8]
        ev.attendees.append({"id": ticket_id, "name": name, "email": email, "checked_in": False})
        self.save()
        print(f"Attendee {name} added to {ev.name} with ticket ID {ticket_id}")

    def list_attendees(self, event_id):
        ev = next((e for e in self.events if e.id == event_id), None)
        if not ev:
            print(f"Event {event_id} not found.")
            return
        if not ev.attendees:
            print("No attendees.")
            return
        print(f"\n👤 Attendees for {ev.name}:")
        for a in ev.attendees:
            status = "✓" if a["checked_in"] else "✗"
            print(f"  {a['id']}: {a['name']} ({a['email']}) - {status}")

    def generate_tickets(self, event_id, output_dir="."):
        ev = next((e for e in self.events if e.id == event_id), None)
        if not ev:
            print(f"Event {event_id} not found.")
            return
        os.makedirs(output_dir, exist_ok=True)
        for a in ev.attendees:
            # QR code data: event_id + ticket_id
            data = f"{ev.id}:{a['id']}"
            qr = qrcode.QRCode(box_size=10, border=2)
            qr.add_data(data)
            qr.make(fit=True)
            img = qr.make_image(fill="black", back_color="white").convert("RGB")
            # Add label with name and event
            # We'll create a new image with extra space for text
            ticket = Image.new("RGB", (img.width + 100, img.height + 60), "white")
            ticket.paste(img, (50, 10))
            draw = ImageDraw.Draw(ticket)
            try:
                font = ImageFont.truetype("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 16)
            except:
                font = ImageFont.load_default()
            draw.text((50, img.height + 20), f"{a['name']} - {ev.name}", fill="black", font=font)
            filename = f"{output_dir}/{a['id']}.png"
            ticket.save(filename)
            print(f"Ticket saved for {a['name']}: {filename}")

    def checkin(self, event_id, ticket_id):
        ev = next((e for e in self.events if e.id == event_id), None)
        if not ev:
            print(f"Event {event_id} not found.")
            return
        for a in ev.attendees:
            if a["id"] == ticket_id:
                if a["checked_in"]:
                    print(f"Attendee {a['name']} already checked in.")
                else:
                    a["checked_in"] = True
                    self.save()
                    print(f"Checked in {a['name']}.")
                return
        print(f"Ticket {ticket_id} not found for this event.")

    def status(self, event_id):
        ev = next((e for e in self.events if e.id == event_id), None)
        if not ev:
            print(f"Event {event_id} not found.")
            return
        total = len(ev.attendees)
        checked = sum(1 for a in ev.attendees if a["checked_in"])
        print(f"\n📊 Event: {ev.name}")
        print(f"  Total attendees: {total}")
        print(f"  Checked in: {checked}")
        print(f"  Remaining: {total - checked}")
        self.list_attendees(event_id)

def main():
    parser = argparse.ArgumentParser(description="Event Organizer with QR Tickets")
    subparsers = parser.add_subparsers(dest="cmd", required=True)

    # Event create
    create_parser = subparsers.add_parser("event")
    create_sub = create_parser.add_subparsers(dest="subcmd", required=True)
    create_sub.add_parser("list")
    create_cmd = create_sub.add_parser("create")
    create_cmd.add_argument("name")
    create_cmd.add_argument("date")
    create_cmd.add_argument("venue")

    # Attendee
    attendee_parser = subparsers.add_parser("attendee")
    attendee_sub = attendee_parser.add_subparsers(dest="subcmd", required=True)
    attendee_sub.add_parser("list")
    add_cmd = attendee_sub.add_parser("add")
    add_cmd.add_argument("event_id")
    add_cmd.add_argument("name")
    add_cmd.add_argument("email")

    # Ticket
    ticket_parser = subparsers.add_parser("ticket")
    ticket_sub = ticket_parser.add_subparsers(dest="subcmd", required=True)
    gen_cmd = ticket_sub.add_parser("generate")
    gen_cmd.add_argument("event_id")
    gen_cmd.add_argument("-o", "--output", default=".")

    # Checkin
    checkin_parser = subparsers.add_parser("checkin")
    checkin_parser.add_argument("event_id")
    checkin_parser.add_argument("ticket_id")

    # Status
    status_parser = subparsers.add_parser("status")
    status_parser.add_argument("event_id")

    args = parser.parse_args()
    org = Organizer()

    if args.cmd == "event":
        if args.subcmd == "list":
            org.list_events()
        elif args.subcmd == "create":
            org.create_event(args.name, args.date, args.venue)
    elif args.cmd == "attendee":
        if args.subcmd == "list":
            org.list_attendees(args.event_id)
        elif args.subcmd == "add":
            org.add_attendee(args.event_id, args.name, args.email)
    elif args.cmd == "ticket":
        if args.subcmd == "generate":
            org.generate_tickets(args.event_id, args.output)
    elif args.cmd == "checkin":
        org.checkin(args.event_id, args.ticket_id)
    elif args.cmd == "status":
        org.status(args.event_id)

if __name__ == "__main__":
    main()
