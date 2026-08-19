// EventOrganizer.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using QRCoder;

class Attendee
{
    [JsonPropertyName("id")] public string Id { get; set; }
    [JsonPropertyName("name")] public string Name { get; set; }
    [JsonPropertyName("email")] public string Email { get; set; }
    [JsonPropertyName("checked_in")] public bool CheckedIn { get; set; }
}

class Event
{
    [JsonPropertyName("id")] public string Id { get; set; }
    [JsonPropertyName("name")] public string Name { get; set; }
    [JsonPropertyName("date")] public string Date { get; set; }
    [JsonPropertyName("venue")] public string Venue { get; set; }
    [JsonPropertyName("attendees")] public List<Attendee> Attendees { get; set; } = new List<Attendee>();
}

class Organizer
{
    private List<Event> events = new List<Event>();
    private readonly string dataFile = "events.json";
    private readonly JsonSerializerOptions options = new JsonSerializerOptions { WriteIndented = true };

    public Organizer() { Load(); }

    private void Load()
    {
        if (!File.Exists(dataFile)) return;
        string json = File.ReadAllText(dataFile);
        events = JsonSerializer.Deserialize<List<Event>>(json) ?? new List<Event>();
    }

    private void Save()
    {
        string json = JsonSerializer.Serialize(events, options);
        File.WriteAllText(dataFile, json);
    }

    private string GenerateId() => Guid.NewGuid().ToString().Substring(0,8);

    public void CreateEvent(string name, string date, string venue)
    {
        var e = new Event { Id = GenerateId(), Name = name, Date = date, Venue = venue };
        events.Add(e);
        Save();
        Console.WriteLine($"Event created: {e.Id} - {e.Name}");
    }

    public void ListEvents()
    {
        if (!events.Any()) { Console.WriteLine("No events."); return; }
        Console.WriteLine("\n📅 Events:");
        foreach (var e in events)
            Console.WriteLine($"  {e.Id}: {e.Name} ({e.Date}) at {e.Venue} - {e.Attendees.Count} attendees");
    }

    public void AddAttendee(string eventId, string name, string email)
    {
        var ev = events.FirstOrDefault(e => e.Id == eventId);
        if (ev == null) { Console.WriteLine($"Event {eventId} not found."); return; }
        var a = new Attendee { Id = GenerateId(), Name = name, Email = email, CheckedIn = false };
        ev.Attendees.Add(a);
        Save();
        Console.WriteLine($"Attendee {name} added with ticket ID {a.Id}");
    }

    public void ListAttendees(string eventId)
    {
        var ev = events.FirstOrDefault(e => e.Id == eventId);
        if (ev == null) { Console.WriteLine($"Event {eventId} not found."); return; }
        if (!ev.Attendees.Any()) { Console.WriteLine("No attendees."); return; }
        Console.WriteLine($"\n👤 Attendees for {ev.Name}:");
        foreach (var a in ev.Attendees)
        {
            string status = a.CheckedIn ? "✓" : "✗";
            Console.WriteLine($"  {a.Id}: {a.Name} ({a.Email}) - {status}");
        }
    }

    public void GenerateTickets(string eventId, string outputDir = ".")
    {
        var ev = events.FirstOrDefault(e => e.Id == eventId);
        if (ev == null) { Console.WriteLine($"Event {eventId} not found."); return; }
        Directory.CreateDirectory(outputDir);
        foreach (var a in ev.Attendees)
        {
            string data = $"{ev.Id}:{a.Id}";
            var qrGenerator = new QRCodeGenerator();
            var qrCodeData = qrGenerator.CreateQrCode(data, QRCodeGenerator.ECCLevel.M);
            var qrCode = new QRCode(qrCodeData);
            var qrImage = qrCode.GetGraphic(6);
            string filename = Path.Combine(outputDir, $"{a.Id}.png");
            qrImage.Save(filename, System.Drawing.Imaging.ImageFormat.Png);
            Console.WriteLine($"Ticket saved for {a.Name}: {filename}");
        }
    }

    public void Checkin(string eventId, string ticketId)
    {
        var ev = events.FirstOrDefault(e => e.Id == eventId);
        if (ev == null) { Console.WriteLine($"Event {eventId} not found."); return; }
        var a = ev.Attendees.FirstOrDefault(att => att.Id == ticketId);
        if (a == null) { Console.WriteLine($"Ticket {ticketId} not found for this event."); return; }
        if (a.CheckedIn) Console.WriteLine($"Attendee {a.Name} already checked in.");
        else { a.CheckedIn = true; Save(); Console.WriteLine($"Checked in {a.Name}."); }
    }

    public void Status(string eventId)
    {
        var ev = events.FirstOrDefault(e => e.Id == eventId);
        if (ev == null) { Console.WriteLine($"Event {eventId} not found."); return; }
        int total = ev.Attendees.Count;
        int checkedIn = ev.Attendees.Count(a => a.CheckedIn);
        Console.WriteLine($"\n📊 Event: {ev.Name}");
        Console.WriteLine($"  Total attendees: {total}");
        Console.WriteLine($"  Checked in: {checkedIn}");
        Console.WriteLine($"  Remaining: {total - checkedIn}");
        ListAttendees(eventId);
    }
}

class Program
{
    static void Main(string[] args)
    {
        if (args.Length < 1) { Console.WriteLine("Usage: EventOrganizer <command> [options]"); return; }
        var app = new Organizer();
        var cmd = args[0];
        switch (cmd)
        {
            case "event":
                if (args.Length < 2) { Console.WriteLine("event list | event create <name> <date> <venue>"); return; }
                if (args[1] == "list") app.ListEvents();
                else if (args[1] == "create" && args.Length == 5)
                    app.CreateEvent(args[2], args[3], args[4]);
                else Console.WriteLine("Invalid event subcommand.");
                break;
            case "attendee":
                if (args.Length < 3) { Console.WriteLine("attendee list <event_id> | attendee add <event_id> <name> <email>"); return; }
                if (args[1] == "list") app.ListAttendees(args[2]);
                else if (args[1] == "add" && args.Length == 5)
                    app.AddAttendee(args[2], args[3], args[4]);
                else Console.WriteLine("Invalid attendee subcommand.");
                break;
            case "ticket":
                if (args.Length < 3 || args[1] != "generate")
                { Console.WriteLine("ticket generate <event_id> [-o output_dir]"); return; }
                string outputDir = ".";
                for (int i=2; i<args.Length; i++)
                {
                    if (args[i] == "-o" && i+1 < args.Length)
                    { outputDir = args[i+1]; i++; }
                }
                app.GenerateTickets(args[2], outputDir);
                break;
            case "checkin":
                if (args.Length < 3) { Console.WriteLine("checkin <event_id> <ticket_id>"); return; }
                app.Checkin(args[1], args[2]);
                break;
            case "status":
                if (args.Length < 2) { Console.WriteLine("status <event_id>"); return; }
                app.Status(args[1]);
                break;
            default:
                Console.WriteLine("Unknown command.");
                break;
        }
    }
}
