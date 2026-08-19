// EventOrganizer.java
import java.io.*;
import java.nio.file.*;
import java.util.*;
import com.google.gson.*;
import com.google.zxing.BarcodeFormat;
import com.google.zxing.client.j2se.MatrixToImageWriter;
import com.google.zxing.common.BitMatrix;
import com.google.zxing.qrcode.QRCodeWriter;

class Attendee {
    String id, name, email;
    boolean checked_in;
}

class Event {
    String id, name, date, venue;
    List<Attendee> attendees = new ArrayList<>();
}

public class EventOrganizer {
    private List<Event> events = new ArrayList<>();
    private final String dataFile = "events.json";
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    public EventOrganizer() { load(); }

    private void load() {
        try {
            Path path = Paths.get(dataFile);
            if (Files.exists(path)) {
                String json = new String(Files.readAllBytes(path));
                Event[] arr = gson.fromJson(json, Event[].class);
                events = Arrays.asList(arr);
            }
        } catch (Exception e) {}
    }

    private void save() {
        try {
            Files.write(Paths.get(dataFile), gson.toJson(events).getBytes());
        } catch (Exception e) {}
    }

    private String generateId() { return UUID.randomUUID().toString().substring(0,8); }

    public void createEvent(String name, String date, String venue) {
        Event e = new Event();
        e.id = generateId();
        e.name = name;
        e.date = date;
        e.venue = venue;
        events.add(e);
        save();
        System.out.printf("Event created: %s - %s%n", e.id, e.name);
    }

    public void listEvents() {
        if (events.isEmpty()) { System.out.println("No events."); return; }
        System.out.println("\n📅 Events:");
        for (Event e : events) {
            System.out.printf("  %s: %s (%s) at %s - %d attendees%n", e.id, e.name, e.date, e.venue, e.attendees.size());
        }
    }

    public void addAttendee(String eventId, String name, String email) {
        for (Event e : events) {
            if (e.id.equals(eventId)) {
                Attendee a = new Attendee();
                a.id = generateId();
                a.name = name;
                a.email = email;
                a.checked_in = false;
                e.attendees.add(a);
                save();
                System.out.printf("Attendee %s added with ticket ID %s%n", name, a.id);
                return;
            }
        }
        System.out.printf("Event %s not found.%n", eventId);
    }

    public void listAttendees(String eventId) {
        for (Event e : events) {
            if (e.id.equals(eventId)) {
                if (e.attendees.isEmpty()) { System.out.println("No attendees."); return; }
                System.out.printf("\n👤 Attendees for %s:%n", e.name);
                for (Attendee a : e.attendees) {
                    String status = a.checked_in ? "✓" : "✗";
                    System.out.printf("  %s: %s (%s) - %s%n", a.id, a.name, a.email, status);
                }
                return;
            }
        }
        System.out.printf("Event %s not found.%n", eventId);
    }

    public void generateTickets(String eventId, String outputDir) {
        for (Event e : events) {
            if (e.id.equals(eventId)) {
                try {
                    Files.createDirectories(Paths.get(outputDir));
                    QRCodeWriter qrWriter = new QRCodeWriter();
                    for (Attendee a : e.attendees) {
                        String data = e.id + ":" + a.id;
                        BitMatrix matrix = qrWriter.encode(data, BarcodeFormat.QR_CODE, 300, 300);
                        Path path = Paths.get(outputDir, a.id + ".png");
                        MatrixToImageWriter.writeToPath(matrix, "PNG", path);
                        System.out.printf("Ticket saved for %s: %s%n", a.name, path.toString());
                    }
                } catch (Exception ex) {
                    System.err.println("Error generating tickets: " + ex.getMessage());
                }
                return;
            }
        }
        System.out.printf("Event %s not found.%n", eventId);
    }

    public void checkin(String eventId, String ticketId) {
        for (Event e : events) {
            if (e.id.equals(eventId)) {
                for (Attendee a : e.attendees) {
                    if (a.id.equals(ticketId)) {
                        if (a.checked_in) {
                            System.out.printf("Attendee %s already checked in.%n", a.name);
                        } else {
                            a.checked_in = true;
                            save();
                            System.out.printf("Checked in %s.%n", a.name);
                        }
                        return;
                    }
                }
                System.out.printf("Ticket %s not found for this event.%n", ticketId);
                return;
            }
        }
        System.out.printf("Event %s not found.%n", eventId);
    }

    public void status(String eventId) {
        for (Event e : events) {
            if (e.id.equals(eventId)) {
                int total = e.attendees.size();
                int checked = 0;
                for (Attendee a : e.attendees) if (a.checked_in) checked++;
                System.out.printf("\n📊 Event: %s%n", e.name);
                System.out.printf("  Total attendees: %d%n", total);
                System.out.printf("  Checked in: %d%n", checked);
                System.out.printf("  Remaining: %d%n", total - checked);
                listAttendees(eventId);
                return;
            }
        }
        System.out.printf("Event %s not found.%n", eventId);
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.out.println("Usage: EventOrganizer <command> [options]");
            return;
        }
        EventOrganizer app = new EventOrganizer();
        String cmd = args[0];
        switch (cmd) {
            case "event":
                if (args.length < 2) { System.out.println("event list | event create <name> <date> <venue>"); return; }
                if (args[1].equals("list")) app.listEvents();
                else if (args[1].equals("create") && args.length == 5)
                    app.createEvent(args[2], args[3], args[4]);
                else System.out.println("Invalid event subcommand.");
                break;
            case "attendee":
                if (args.length < 3) { System.out.println("attendee list <event_id> | attendee add <event_id> <name> <email>"); return; }
                if (args[1].equals("list")) app.listAttendees(args[2]);
                else if (args[1].equals("add") && args.length == 5)
                    app.addAttendee(args[2], args[3], args[4]);
                else System.out.println("Invalid attendee subcommand.");
                break;
            case "ticket":
                if (args.length < 3 || !args[1].equals("generate")) {
                    System.out.println("ticket generate <event_id> [-o output_dir]");
                    return;
                }
                String outputDir = ".";
                for (int i = 2; i < args.length; i++) {
                    if (args[i].equals("-o") && i+1 < args.length) {
                        outputDir = args[i+1];
                        i++;
                    }
                }
                app.generateTickets(args[2], outputDir);
                break;
            case "checkin":
                if (args.length < 3) { System.out.println("checkin <event_id> <ticket_id>"); return; }
                app.checkin(args[1], args[2]);
                break;
            case "status":
                if (args.length < 2) { System.out.println("status <event_id>"); return; }
                app.status(args[1]);
                break;
            default:
                System.out.println("Unknown command.");
        }
    }
}
