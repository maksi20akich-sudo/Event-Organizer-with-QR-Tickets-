// event_organizer.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <random>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <qrencode.h> // libqrencode

using namespace std;
using json = nlohmann::json;

struct Attendee {
    string id, name, email;
    bool checked_in;
};

struct Event {
    string id, name, date, venue;
    vector<Attendee> attendees;
};

class Organizer {
private:
    vector<Event> events;
    string dataFile = "events.json";

    string generateId() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 15);
        stringstream ss;
        for (int i = 0; i < 8; i++) ss << hex << dis(gen);
        return ss.str();
    }

    void load() {
        ifstream f(dataFile);
        if (!f.is_open()) return;
        json j;
        f >> j;
        for (auto& item : j) {
            Event e;
            e.id = item["id"];
            e.name = item["name"];
            e.date = item["date"];
            e.venue = item["venue"];
            for (auto& att : item["attendees"]) {
                Attendee a;
                a.id = att["id"];
                a.name = att["name"];
                a.email = att["email"];
                a.checked_in = att["checked_in"];
                e.attendees.push_back(a);
            }
            events.push_back(e);
        }
    }

    void save() {
        json j = json::array();
        for (auto& e : events) {
            json attendees = json::array();
            for (auto& a : e.attendees) {
                attendees.push_back({{"id", a.id}, {"name", a.name}, {"email", a.email}, {"checked_in", a.checked_in}});
            }
            j.push_back({{"id", e.id}, {"name", e.name}, {"date", e.date}, {"venue", e.venue}, {"attendees", attendees}});
        }
        ofstream f(dataFile);
        f << setw(2) << j << endl;
    }

public:
    Organizer() { load(); }

    void createEvent(const string& name, const string& date, const string& venue) {
        Event e{generateId(), name, date, venue, {}};
        events.push_back(e);
        save();
        cout << "Event created: " << e.id << " - " << e.name << endl;
    }

    void listEvents() {
        if (events.empty()) { cout << "No events.\n"; return; }
        cout << "\n📅 Events:\n";
        for (auto& e : events) {
            cout << "  " << e.id << ": " << e.name << " (" << e.date << ") at " << e.venue << " - " << e.attendees.size() << " attendees\n";
        }
    }

    void addAttendee(const string& eventId, const string& name, const string& email) {
        for (auto& e : events) {
            if (e.id == eventId) {
                Attendee a{generateId(), name, email, false};
                e.attendees.push_back(a);
                save();
                cout << "Attendee " << name << " added with ticket ID " << a.id << endl;
                return;
            }
        }
        cout << "Event " << eventId << " not found.\n";
    }

    void listAttendees(const string& eventId) {
        for (auto& e : events) {
            if (e.id == eventId) {
                if (e.attendees.empty()) { cout << "No attendees.\n"; return; }
                cout << "\n👤 Attendees for " << e.name << ":\n";
                for (auto& a : e.attendees) {
                    cout << "  " << a.id << ": " << a.name << " (" << a.email << ") - " << (a.checked_in ? "✓" : "✗") << endl;
                }
                return;
            }
        }
        cout << "Event " << eventId << " not found.\n";
    }

    void generateTickets(const string& eventId, const string& outputDir = ".") {
        for (auto& e : events) {
            if (e.id == eventId) {
                system(("mkdir -p " + outputDir).c_str());
                for (auto& a : e.attendees) {
                    string data = e.id + ":" + a.id;
                    QRcode* qr = QRcode_encodeString(data.c_str(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
                    if (!qr) { cerr << "QR generation failed for " << a.name << endl; continue; }
                    int size = qr->width;
                    int margin = 2;
                    int moduleSize = 6;
                    int imgSize = (size + 2*margin) * moduleSize;
                    // Write PPM (simplest) - we'll save as .ppm, user can convert
                    string filename = outputDir + "/" + a.id + ".ppm";
                    ofstream f(filename);
                    f << "P3\n" << imgSize << " " << imgSize << "\n255\n";
                    for (int y = -margin; y < size + margin; y++) {
                        for (int x = -margin; x < size + margin; x++) {
                            bool isBlack = false;
                            if (x >= 0 && x < size && y >= 0 && y < size)
                                isBlack = qr->data[y*size + x] & 1;
                            if (isBlack) f << "0 0 0 ";
                            else f << "255 255 255 ";
                        }
                        f << "\n";
                    }
                    f.close();
                    QRcode_free(qr);
                    cout << "Ticket saved for " << a.name << ": " << filename << endl;
                }
                return;
            }
        }
        cout << "Event " << eventId << " not found.\n";
    }

    void checkin(const string& eventId, const string& ticketId) {
        for (auto& e : events) {
            if (e.id == eventId) {
                for (auto& a : e.attendees) {
                    if (a.id == ticketId) {
                        if (a.checked_in) cout << "Attendee " << a.name << " already checked in.\n";
                        else { a.checked_in = true; save(); cout << "Checked in " << a.name << ".\n"; }
                        return;
                    }
                }
                cout << "Ticket " << ticketId << " not found for this event.\n";
                return;
            }
        }
        cout << "Event " << eventId << " not found.\n";
    }

    void status(const string& eventId) {
        for (auto& e : events) {
            if (e.id == eventId) {
                int total = e.attendees.size();
                int checked = 0;
                for (auto& a : e.attendees) if (a.checked_in) checked++;
                cout << "\n📊 Event: " << e.name << endl;
                cout << "  Total attendees: " << total << endl;
                cout << "  Checked in: " << checked << endl;
                cout << "  Remaining: " << total - checked << endl;
                listAttendees(eventId);
                return;
            }
        }
        cout << "Event " << eventId << " not found.\n";
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) { cerr << "Usage: event_organizer <command> [options]\n"; return 1; }
    Organizer app;
    string cmd = argv[1];
    if (cmd == "event") {
        if (argc < 3) { cerr << "event list | event create <name> <date> <venue>\n"; return 1; }
        if (string(argv[2]) == "list") app.listEvents();
        else if (string(argv[2]) == "create" && argc >= 6) {
            app.createEvent(argv[3], argv[4], argv[5]);
        } else cerr << "Invalid event subcommand.\n";
    } else if (cmd == "attendee") {
        if (argc < 4) { cerr << "attendee list <event_id> | attendee add <event_id> <name> <email>\n"; return 1; }
        if (string(argv[2]) == "list") app.listAttendees(argv[3]);
        else if (string(argv[2]) == "add" && argc >= 6) {
            app.addAttendee(argv[3], argv[4], argv[5]);
        } else cerr << "Invalid attendee subcommand.\n";
    } else if (cmd == "ticket") {
        if (argc < 4 || string(argv[2]) != "generate") {
            cerr << "ticket generate <event_id> [-o output_dir]\n"; return 1;
        }
        string outputDir = ".";
        for (int i=3; i<argc; i++) {
            if (string(argv[i]) == "-o" && i+1 < argc) { outputDir = argv[++i]; }
        }
        app.generateTickets(argv[3], outputDir);
    } else if (cmd == "checkin") {
        if (argc < 4) { cerr << "checkin <event_id> <ticket_id>\n"; return 1; }
        app.checkin(argv[2], argv[3]);
    } else if (cmd == "status") {
        if (argc < 3) { cerr << "status <event_id>\n"; return 1; }
        app.status(argv[2]);
    } else {
        cerr << "Unknown command.\n";
    }
    return 0;
}
