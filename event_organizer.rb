# event_organizer.rb
require 'json'
require 'securerandom'
require 'optparse'
require 'rqrcode'
require 'chunky_png'

DATA_FILE = 'events.json'

class Event
  attr_accessor :id, :name, :date, :venue, :attendees

  def initialize(name, date, venue)
    @id = SecureRandom.hex(4)
    @name = name
    @date = date
    @venue = venue
    @attendees = []
  end

  def to_hash
    { id: @id, name: @name, date: @date, venue: @venue, attendees: @attendees }
  end

  def self.from_hash(h)
    e = new(h['name'], h['date'], h['venue'])
    e.id = h['id']
    e.attendees = h['attendees']
    e
  end
end

class Organizer
  attr_reader :events

  def initialize
    @events = []
    load
  end

  def load
    return unless File.exist?(DATA_FILE)
    data = JSON.parse(File.read(DATA_FILE))
    @events = data.map { |h| Event.from_hash(h) }
  end

  def save
    File.write(DATA_FILE, JSON.pretty_generate(@events.map(&:to_hash)))
  end

  def create_event(name, date, venue)
    ev = Event.new(name, date, venue)
    @events << ev
    save
    puts "Event created: #{ev.id} - #{name}"
  end

  def list_events
    if @events.empty?
      puts "No events."
      return
    end
    puts "\n📅 Events:"
    @events.each do |e|
      puts "  #{e.id}: #{e.name} (#{e.date}) at #{e.venue} - #{e.attendees.size} attendees"
    end
  end

  def add_attendee(event_id, name, email)
    ev = @events.find { |e| e.id == event_id }
    unless ev
      puts "Event #{event_id} not found."
      return
    end
    ticket_id = SecureRandom.hex(4)
    ev.attendees << { "id" => ticket_id, "name" => name, "email" => email, "checked_in" => false }
    save
    puts "Attendee #{name} added with ticket ID #{ticket_id}"
  end

  def list_attendees(event_id)
    ev = @events.find { |e| e.id == event_id }
    unless ev
      puts "Event #{event_id} not found."
      return
    end
    if ev.attendees.empty?
      puts "No attendees."
      return
    end
    puts "\n👤 Attendees for #{ev.name}:"
    ev.attendees.each do |a|
      status = a["checked_in"] ? "✓" : "✗"
      puts "  #{a['id']}: #{a['name']} (#{a['email']}) - #{status}"
    end
  end

  def generate_tickets(event_id, output_dir = ".")
    ev = @events.find { |e| e.id == event_id }
    unless ev
      puts "Event #{event_id} not found."
      return
    end
    Dir.mkdir(output_dir) unless Dir.exist?(output_dir)
    ev.attendees.each do |a|
      data = "#{ev.id}:#{a['id']}"
      qr = RQRCode::QRCode.new(data, level: :m)
      png = qr.as_png(border_modules: 2, module_px_size: 6)
      filename = "#{output_dir}/#{a['id']}.png"
      png.save(filename, interlace: true)
      puts "Ticket saved for #{a['name']}: #{filename}"
    end
  end

  def checkin(event_id, ticket_id)
    ev = @events.find { |e| e.id == event_id }
    unless ev
      puts "Event #{event_id} not found."
      return
    end
    a = ev.attendees.find { |att| att['id'] == ticket_id }
    unless a
      puts "Ticket #{ticket_id} not found for this event."
      return
    end
    if a['checked_in']
      puts "Attendee #{a['name']} already checked in."
    else
      a['checked_in'] = true
      save
      puts "Checked in #{a['name']}."
    end
  end

  def status(event_id)
    ev = @events.find { |e| e.id == event_id }
    unless ev
      puts "Event #{event_id} not found."
      return
    end
    total = ev.attendees.size
    checked = ev.attendees.count { |a| a['checked_in'] }
    puts "\n📊 Event: #{ev.name}"
    puts "  Total attendees: #{total}"
    puts "  Checked in: #{checked}"
    puts "  Remaining: #{total - checked}"
    list_attendees(event_id)
  end
end

options = {}
$command = ARGV.shift
if $command.nil?
  puts "Usage: event_organizer.rb <command> [options]"
  exit 1
end

app = Organizer.new

case $command
when "event"
  sub = ARGV.shift
  if sub == "list"
    app.list_events
  elsif sub == "create"
    name, date, venue = ARGV.shift(3)
    app.create_event(name, date, venue)
  else
    puts "Unknown event subcommand"
  end
when "attendee"
  sub = ARGV.shift
  if sub == "list"
    event_id = ARGV.shift
    app.list_attendees(event_id)
  elsif sub == "add"
    event_id, name, email = ARGV.shift(3)
    app.add_attendee(event_id, name, email)
  else
    puts "Unknown attendee subcommand"
  end
when "ticket"
  sub = ARGV.shift
  if sub == "generate"
    event_id = ARGV.shift
    output_dir = "."
    if ARGV.include?("-o")
      idx = ARGV.index("-o")
      output_dir = ARGV[idx+1] if idx+1 < ARGV.size
    end
    app.generate_tickets(event_id, output_dir)
  else
    puts "Unknown ticket subcommand"
  end
when "checkin"
  event_id, ticket_id = ARGV.shift(2)
  app.checkin(event_id, ticket_id)
when "status"
  event_id = ARGV.shift
  app.status(event_id)
else
  puts "Unknown command"
end
