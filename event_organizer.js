// event_organizer.js
#!/usr/bin/env node
const fs = require('fs');
const path = require('path');
const { program } = require('commander');
const { v4: uuidv4 } = require('uuid');
const QRCode = require('qrcode');

const DATA_FILE = 'events.json';

class Event {
    constructor(name, date, venue) {
        this.id = uuidv4().slice(0,8);
        this.name = name;
        this.date = date;
        this.venue = venue;
        this.attendees = [];
    }
}

class Organizer {
    constructor() {
        this.events = [];
        this.load();
    }

    load() {
        if (fs.existsSync(DATA_FILE)) {
            const data = JSON.parse(fs.readFileSync(DATA_FILE));
            this.events = data.map(e => Object.assign(new Event(e.name, e.date, e.venue), e));
        }
    }

    save() {
        fs.writeFileSync(DATA_FILE, JSON.stringify(this.events, null, 2));
    }

    createEvent(name, date, venue) {
        const ev = new Event(name, date, venue);
        this.events.push(ev);
        this.save();
        console.log(`Event created: ${ev.id} - ${name}`);
    }

    listEvents() {
        if (this.events.length === 0) {
            console.log('No events.');
            return;
        }
        console.log('\n📅 Events:');
        for (const e of this.events) {
            console.log(`  ${e.id}: ${e.name} (${e.date}) at ${e.venue} - ${e.attendees.length} attendees`);
        }
    }

    addAttendee(eventId, name, email) {
        const ev = this.events.find(e => e.id === eventId);
        if (!ev) {
            console.log(`Event ${eventId} not found.`);
            return;
        }
        const ticketId = uuidv4().slice(0,8);
        ev.attendees.push({ id: ticketId, name, email, checked_in: false });
        this.save();
        console.log(`Attendee ${name} added with ticket ID ${ticketId}`);
    }

    listAttendees(eventId) {
        const ev = this.events.find(e => e.id === eventId);
        if (!ev) {
            console.log(`Event ${eventId} not found.`);
            return;
        }
        if (ev.attendees.length === 0) {
            console.log('No attendees.');
            return;
        }
        console.log(`\n👤 Attendees for ${ev.name}:`);
        for (const a of ev.attendees) {
            const status = a.checked_in ? '✓' : '✗';
            console.log(`  ${a.id}: ${a.name} (${a.email}) - ${status}`);
        }
    }

    async generateTickets(eventId, outputDir = '.') {
        const ev = this.events.find(e => e.id === eventId);
        if (!ev) {
            console.log(`Event ${eventId} not found.`);
            return;
        }
        fs.mkdirSync(outputDir, { recursive: true });
        for (const a of ev.attendees) {
            const data = `${ev.id}:${a.id}`;
            const filename = path.join(outputDir, `${a.id}.png`);
            await QRCode.toFile(filename, data, { width: 300, margin: 2 });
            console.log(`Ticket saved for ${a.name}: ${filename}`);
        }
    }

    checkin(eventId, ticketId) {
        const ev = this.events.find(e => e.id === eventId);
        if (!ev) {
            console.log(`Event ${eventId} not found.`);
            return;
        }
        const attendee = ev.attendees.find(a => a.id === ticketId);
        if (!attendee) {
            console.log(`Ticket ${ticketId} not found for this event.`);
            return;
        }
        if (attendee.checked_in) {
            console.log(`Attendee ${attendee.name} already checked in.`);
        } else {
            attendee.checked_in = true;
            this.save();
            console.log(`Checked in ${attendee.name}.`);
        }
    }

    status(eventId) {
        const ev = this.events.find(e => e.id === eventId);
        if (!ev) {
            console.log(`Event ${eventId} not found.`);
            return;
        }
        const total = ev.attendees.length;
        const checked = ev.attendees.filter(a => a.checked_in).length;
        console.log(`\n📊 Event: ${ev.name}`);
        console.log(`  Total attendees: ${total}`);
        console.log(`  Checked in: ${checked}`);
        console.log(`  Remaining: ${total - checked}`);
        this.listAttendees(eventId);
    }
}

program
    .command('event')
    .option('create <name> <date> <venue>', 'Create event')
    .option('list', 'List events')
    .action((options) => {
        const org = new Organizer();
        // Handle subcommands based on arguments
        const args = program.args.slice(1); // after 'event'
        if (args[0] === 'list') {
            org.listEvents();
        } else if (args[0] === 'create' && args.length === 4) {
            org.createEvent(args[1], args[2], args[3]);
        } else {
            console.log('Usage: event list | event create <name> <date> <venue>');
        }
    });

program
    .command('attendee')
    .option('list <event_id>', 'List attendees')
    .option('add <event_id> <name> <email>', 'Add attendee')
    .action((options) => {
        const org = new Organizer();
        const args = program.args.slice(1);
        if (args[0] === 'list' && args.length === 2) {
            org.listAttendees(args[1]);
        } else if (args[0] === 'add' && args.length === 4) {
            org.addAttendee(args[1], args[2], args[3]);
        } else {
            console.log('Usage: attendee list <event_id> | attendee add <event_id> <name> <email>');
        }
    });

program
    .command('ticket')
    .option('generate <event_id> [-o output_dir]', 'Generate tickets')
    .action((options) => {
        const org = new Organizer();
        const args = program.args.slice(1);
        if (args[0] === 'generate' && args.length >= 2) {
            let outputDir = '.';
            if (args.includes('-o')) {
                const idx = args.indexOf('-o');
                if (idx + 1 < args.length) outputDir = args[idx+1];
            }
            org.generateTickets(args[1], outputDir);
        } else {
            console.log('Usage: ticket generate <event_id> [-o output_dir]');
        }
    });

program
    .command('checkin <event_id> <ticket_id>')
    .action((eventId, ticketId) => {
        const org = new Organizer();
        org.checkin(eventId, ticketId);
    });

program
    .command('status <event_id>')
    .action((eventId) => {
        const org = new Organizer();
        org.status(eventId);
    });

program.parse(process.argv);
