// event_organizer.go
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"strings"
	"time"
	"github.com/google/uuid"
	"github.com/skip2/go-qrcode"
)

type Attendee struct {
	ID        string `json:"id"`
	Name      string `json:"name"`
	Email     string `json:"email"`
	CheckedIn bool   `json:"checked_in"`
}

type Event struct {
	ID        string     `json:"id"`
	Name      string     `json:"name"`
	Date      string     `json:"date"`
	Venue     string     `json:"venue"`
	Attendees []Attendee `json:"attendees"`
}

type Organizer struct {
	Events []Event `json:"events"`
	File   string
}

func NewOrganizer(file string) *Organizer {
	o := &Organizer{File: file}
	o.load()
	return o
}

func (o *Organizer) load() {
	data, err := os.ReadFile(o.File)
	if err != nil {
		return
	}
	json.Unmarshal(data, o)
}

func (o *Organizer) save() {
	data, _ := json.MarshalIndent(o, "", "  ")
	os.WriteFile(o.File, data, 0644)
}

func (o *Organizer) CreateEvent(name, date, venue string) {
	ev := Event{
		ID:    uuid.New().String()[:8],
		Name:  name,
		Date:  date,
		Venue: venue,
	}
	o.Events = append(o.Events, ev)
	o.save()
	fmt.Printf("Event created: %s - %s\n", ev.ID, ev.Name)
}

func (o *Organizer) ListEvents() {
	if len(o.Events) == 0 {
		fmt.Println("No events.")
		return
	}
	fmt.Println("\n📅 Events:")
	for _, e := range o.Events {
		fmt.Printf("  %s: %s (%s) at %s - %d attendees\n", e.ID, e.Name, e.Date, e.Venue, len(e.Attendees))
	}
}

func (o *Organizer) AddAttendee(eventID, name, email string) {
	for i, ev := range o.Events {
		if ev.ID == eventID {
			a := Attendee{
				ID:        uuid.New().String()[:8],
				Name:      name,
				Email:     email,
				CheckedIn: false,
			}
			o.Events[i].Attendees = append(o.Events[i].Attendees, a)
			o.save()
			fmt.Printf("Attendee %s added with ticket ID %s\n", name, a.ID)
			return
		}
	}
	fmt.Printf("Event %s not found.\n", eventID)
}

func (o *Organizer) ListAttendees(eventID string) {
	for _, ev := range o.Events {
		if ev.ID == eventID {
			if len(ev.Attendees) == 0 {
				fmt.Println("No attendees.")
				return
			}
			fmt.Printf("\n👤 Attendees for %s:\n", ev.Name)
			for _, a := range ev.Attendees {
				status := "✗"
				if a.CheckedIn {
					status = "✓"
				}
				fmt.Printf("  %s: %s (%s) - %s\n", a.ID, a.Name, a.Email, status)
			}
			return
		}
	}
	fmt.Printf("Event %s not found.\n", eventID)
}

func (o *Organizer) GenerateTickets(eventID, outputDir string) {
	for _, ev := range o.Events {
		if ev.ID == eventID {
			if err := os.MkdirAll(outputDir, 0755); err != nil {
				fmt.Println("Error creating output dir:", err)
				return
			}
			for _, a := range ev.Attendees {
				data := fmt.Sprintf("%s:%s", ev.ID, a.ID)
				filename := fmt.Sprintf("%s/%s.png", outputDir, a.ID)
				err := qrcode.WriteFile(data, qrcode.Medium, 256, filename)
				if err != nil {
					fmt.Printf("Error generating ticket for %s: %v\n", a.Name, err)
				} else {
					fmt.Printf("Ticket saved for %s: %s\n", a.Name, filename)
				}
			}
			return
		}
	}
	fmt.Printf("Event %s not found.\n", eventID)
}

func (o *Organizer) Checkin(eventID, ticketID string) {
	for i, ev := range o.Events {
		if ev.ID == eventID {
			for j, a := range ev.Attendees {
				if a.ID == ticketID {
					if a.CheckedIn {
						fmt.Printf("Attendee %s already checked in.\n", a.Name)
					} else {
						o.Events[i].Attendees[j].CheckedIn = true
						o.save()
						fmt.Printf("Checked in %s.\n", a.Name)
					}
					return
				}
			}
			fmt.Printf("Ticket %s not found for this event.\n", ticketID)
			return
		}
	}
	fmt.Printf("Event %s not found.\n", eventID)
}

func (o *Organizer) Status(eventID string) {
	for _, ev := range o.Events {
		if ev.ID == eventID {
			total := len(ev.Attendees)
			checked := 0
			for _, a := range ev.Attendees {
				if a.CheckedIn {
					checked++
				}
			}
			fmt.Printf("\n📊 Event: %s\n", ev.Name)
			fmt.Printf("  Total attendees: %d\n", total)
			fmt.Printf("  Checked in: %d\n", checked)
			fmt.Printf("  Remaining: %d\n", total-checked)
			o.ListAttendees(eventID)
			return
		}
	}
	fmt.Printf("Event %s not found.\n", eventID)
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: event_organizer <command> [options]")
		return
	}
	org := NewOrganizer("events.json")
	switch os.Args[1] {
	case "event":
		if len(os.Args) < 3 {
			fmt.Println("Usage: event create <name> <date> <venue> | event list")
			return
		}
		if os.Args[2] == "list" {
			org.ListEvents()
		} else if os.Args[2] == "create" && len(os.Args) >= 6 {
			org.CreateEvent(os.Args[3], os.Args[4], os.Args[5])
		} else {
			fmt.Println("Invalid event command.")
		}
	case "attendee":
		if len(os.Args) < 4 {
			fmt.Println("Usage: attendee add <event_id> <name> <email> | attendee list <event_id>")
			return
		}
		if os.Args[2] == "list" {
			org.ListAttendees(os.Args[3])
		} else if os.Args[2] == "add" && len(os.Args) >= 6 {
			org.AddAttendee(os.Args[3], os.Args[4], os.Args[5])
		} else {
			fmt.Println("Invalid attendee command.")
		}
	case "ticket":
		if len(os.Args) < 4 {
			fmt.Println("Usage: ticket generate <event_id> [-o output_dir]")
			return
		}
		if os.Args[2] == "generate" {
			outputDir := "."
			for i := 3; i < len(os.Args); i++ {
				if os.Args[i] == "-o" && i+1 < len(os.Args) {
					outputDir = os.Args[i+1]
					i++
				}
			}
			org.GenerateTickets(os.Args[3], outputDir)
		}
	case "checkin":
		if len(os.Args) < 4 {
			fmt.Println("Usage: checkin <event_id> <ticket_id>")
			return
		}
		org.Checkin(os.Args[2], os.Args[3])
	case "status":
		if len(os.Args) < 3 {
			fmt.Println("Usage: status <event_id>")
			return
		}
		org.Status(os.Args[2])
	default:
		fmt.Println("Unknown command.")
	}
}
