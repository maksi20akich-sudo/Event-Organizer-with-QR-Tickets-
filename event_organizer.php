# event_organizer.php
<?php
require_once('phpqrcode/qrlib.php'); // Download from https://sourceforge.net/projects/phpqrcode/

$dataFile = 'events.json';

class Event {
    public $id, $name, $date, $venue, $attendees;
    function __construct($name, $date, $venue) {
        $this->id = substr(bin2hex(random_bytes(4)), 0, 8);
        $this->name = $name;
        $this->date = $date;
        $this->venue = $venue;
        $this->attendees = [];
    }
}

class Organizer {
    private $events = [];
    private $file;

    function __construct($file) {
        $this->file = $file;
        $this->load();
    }

    function load() {
        if (file_exists($this->file)) {
            $data = json_decode(file_get_contents($this->file), true);
            foreach ($data as $item) {
                $e = new Event($item['name'], $item['date'], $item['venue']);
                $e->id = $item['id'];
                $e->attendees = $item['attendees'];
                $this->events[] = $e;
            }
        }
    }

    function save() {
        $data = [];
        foreach ($this->events as $e) {
            $data[] = [
                'id' => $e->id,
                'name' => $e->name,
                'date' => $e->date,
                'venue' => $e->venue,
                'attendees' => $e->attendees
            ];
        }
        file_put_contents($this->file, json_encode($data, JSON_PRETTY_PRINT));
    }

    function createEvent($name, $date, $venue) {
        $e = new Event($name, $date, $venue);
        $this->events[] = $e;
        $this->save();
        echo "Event created: {$e->id} - $name\n";
    }

    function listEvents() {
        if (empty($this->events)) {
            echo "No events.\n";
            return;
        }
        echo "\n📅 Events:\n";
        foreach ($this->events as $e) {
            echo "  {$e->id}: {$e->name} ({$e->date}) at {$e->venue} - " . count($e->attendees) . " attendees\n";
        }
    }

    function addAttendee($eventId, $name, $email) {
        foreach ($this->events as $e) {
            if ($e->id == $eventId) {
                $ticketId = substr(bin2hex(random_bytes(4)), 0, 8);
                $e->attendees[] = [
                    'id' => $ticketId,
                    'name' => $name,
                    'email' => $email,
                    'checked_in' => false
                ];
                $this->save();
                echo "Attendee $name added with ticket ID $ticketId\n";
                return;
            }
        }
        echo "Event $eventId not found.\n";
    }

    function listAttendees($eventId) {
        foreach ($this->events as $e) {
            if ($e->id == $eventId) {
                if (empty($e->attendees)) {
                    echo "No attendees.\n";
                    return;
                }
                echo "\n👤 Attendees for {$e->name}:\n";
                foreach ($e->attendees as $a) {
                    $status = $a['checked_in'] ? '✓' : '✗';
                    echo "  {$a['id']}: {$a['name']} ({$a['email']}) - $status\n";
                }
                return;
            }
        }
        echo "Event $eventId not found.\n";
    }

    function generateTickets($eventId, $outputDir = '.') {
        foreach ($this->events as $e) {
            if ($e->id == $eventId) {
                if (!is_dir($outputDir)) mkdir($outputDir, 0777, true);
                foreach ($e->attendees as $a) {
                    $data = "{$e->id}:{$a['id']}";
                    $filename = "$outputDir/{$a['id']}.png";
                    QRcode::png($data, $filename, QR_ECLEVEL_M, 6, 2);
                    echo "Ticket saved for {$a['name']}: $filename\n";
                }
                return;
            }
        }
        echo "Event $eventId not found.\n";
    }

    function checkin($eventId, $ticketId) {
        foreach ($this->events as $e) {
            if ($e->id == $eventId) {
                foreach ($e->attendees as &$a) {
                    if ($a['id'] == $ticketId) {
                        if ($a['checked_in']) {
                            echo "Attendee {$a['name']} already checked in.\n";
                        } else {
                            $a['checked_in'] = true;
                            $this->save();
                            echo "Checked in {$a['name']}.\n";
                        }
                        return;
                    }
                }
                echo "Ticket $ticketId not found for this event.\n";
                return;
            }
        }
        echo "Event $eventId not found.\n";
    }

    function status($eventId) {
        foreach ($this->events as $e) {
            if ($e->id == $eventId) {
                $total = count($e->attendees);
                $checked = 0;
                foreach ($e->attendees as $a) if ($a['checked_in']) $checked++;
                echo "\n📊 Event: {$e->name}\n";
                echo "  Total attendees: $total\n";
                echo "  Checked in: $checked\n";
                echo "  Remaining: " . ($total - $checked) . "\n";
                $this->listAttendees($eventId);
                return;
            }
        }
        echo "Event $eventId not found.\n";
    }
}

if ($argc < 2) {
    die("Usage: php event_organizer.php <command> [options]\n");
}
$app = new Organizer($dataFile);
$cmd = $argv[1];

switch ($cmd) {
    case 'event':
        if ($argc < 3) die("event: list or create <name> <date> <venue>\n");
        if ($argv[2] == 'list') {
            $app->listEvents();
        } elseif ($argv[2] == 'create' && $argc >= 6) {
            $app->createEvent($argv[3], $argv[4], $argv[5]);
        } else {
            echo "Invalid event subcommand.\n";
        }
        break;
    case 'attendee':
        if ($argc < 4) die("attendee: list <event_id> or add <event_id> <name> <email>\n");
        if ($argv[2] == 'list') {
            $app->listAttendees($argv[3]);
        } elseif ($argv[2] == 'add' && $argc >= 6) {
            $app->addAttendee($argv[3], $argv[4], $argv[5]);
        } else {
            echo "Invalid attendee subcommand.\n";
        }
        break;
    case 'ticket':
        if ($argc < 3 || $argv[2] != 'generate') die("ticket generate <event_id> [-o output_dir]\n");
        $outputDir = '.';
        for ($i=3; $i<$argc; $i++) {
            if ($argv[$i] == '-o' && isset($argv[$i+1])) {
                $outputDir = $argv[$i+1];
                $i++;
            }
        }
        $app->generateTickets($argv[3], $outputDir);
        break;
    case 'checkin':
        if ($argc < 4) die("checkin <event_id> <ticket_id>\n");
        $app->checkin($argv[2], $argv[3]);
        break;
    case 'status':
        if ($argc < 3) die("status <event_id>\n");
        $app->status($argv[2]);
        break;
    default:
        echo "Unknown command.\n";
}
?>
