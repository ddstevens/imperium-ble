package main

import (
    "fmt"
	"tinygo.org/x/bluetooth"
	"github.com/lxzan/gws"
	"github.com/google/uuid"
)

var (
	adapter = bluetooth.DefaultAdapter
)

func main() {
	println("enabling")

	zaparooServiceUUID, _ := bluetooth.ParseUUID("CE00299E-EA4B-4BB6-B631-A93F4F16E71B")
	zaparooCharacteristicUUID, _ := bluetooth.ParseUUID("8CD024AE-4EA5-4F06-9836-D5CA72976A40")

	socket, _, err := gws.NewClient(new(WebSocket), &gws.ClientOption{
		Addr: "ws://127.0.0.1:7497/",
		PermessageDeflate: gws.PermessageDeflate{
			Enabled:               true,
			ServerContextTakeover: true,
			ClientContextTakeover: true,
		},
	})

	if err != nil {
		println(err.Error())
		return
	}

	go socket.ReadLoop()

	// Enable BLE interface.
	must("enable BLE stack", adapter.Enable())

	ch := make(chan bluetooth.ScanResult, 1)

	// Start scanning.
	println("scanning...")
	err = adapter.Scan(func(adapter *bluetooth.Adapter, result bluetooth.ScanResult) {
		// ********** REPLACE WITH YOUR MCU's MAC ADDRESS **********
		if result.Address.String() == "24:EC:4A:2A:D2:59" || result.Address.String() == "64:E8:33:86:3A:92" {
			adapter.StopScan()
			ch <- result
		}
	})

	var device bluetooth.Device
	select {
	case result := <-ch:
		device, err = adapter.Connect(result.Address, bluetooth.ConnectionParams{})
		if err != nil {
			println(err.Error())
			return
		}

		println("connected to ", result.Address.String())
	}

	// get services
	println("discovering services/characteristics")
	srvcs, err := device.DiscoverServices([]bluetooth.UUID{zaparooServiceUUID})
	must("discover services", err)

	if len(srvcs) == 0 {
		panic("could not find zaparoo service")
	}

	srvc := srvcs[0]

	println("found service", srvc.UUID().String())

	chars, err := srvc.DiscoverCharacteristics([]bluetooth.UUID{zaparooCharacteristicUUID})
	if err != nil {
		println(err)
	}

	if len(chars) == 0 {
		panic("could not find zaparoo characteristic")
	}

	char := chars[0]
	println("found characteristic", char.UUID().String())

	char.EnableNotifications(func(buf []byte) {
		println("data:", string(buf))
        var id = uuid.New()
        var text = fmt.Sprintf(
                `{"jsonrpc": "2.0", "id": "%s", "method": "launch", "params": {"text": "%s", "uuid": ""}}`,
                id, string(buf))

        println("json: ", text)
        socket.WriteString(text)
	})

	select {}
}

func must(action string, err error) {
	if err != nil {
		panic("failed to " + action + ": " + err.Error())
	}
}

type WebSocket struct {
}

func (c *WebSocket) OnClose(socket *gws.Conn, err error) {
	fmt.Printf("onerror: err=%s\n", err.Error())
}

func (c *WebSocket) OnPong(socket *gws.Conn, payload []byte) {
}

func (c *WebSocket) OnOpen(socket *gws.Conn) {
	_ = socket.WriteString("hello, there is client")
}

func (c *WebSocket) OnPing(socket *gws.Conn, payload []byte) {
	_ = socket.WritePong(payload)
}

func (c *WebSocket) OnMessage(socket *gws.Conn, message *gws.Message) {
	defer message.Close()
	fmt.Printf("recv: %s\n", message.Data.String())
}
