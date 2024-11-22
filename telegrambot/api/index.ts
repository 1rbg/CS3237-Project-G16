require("dotenv").config();
import { sql } from "@vercel/postgres";
const bodyParser = require("body-parser");
const axios = require("axios");
const express = require("express");
const app = express();
const TelegramBot = require("node-telegram-bot-api");
const port = process.env.PORT || 8000;
const token = process.env.TELEBOTKEY || "";
const bot = new TelegramBot(token, { polling: false });
const TELEGRAM_API_URL = `https://api.telegram.org/bot${token}`;

// Middleware to parse incoming request bodies
app.use(bodyParser.json());

app.get("/report", async (req, res) => {
    try {
        const { rows } = await sql`SELECT * FROM chatids`;

        // Check if rows exist
        if (rows.length === 0) {
            return res.send("No chat IDs found.");
        }

        // Prepare to send messages
        const messagePromises = rows.map(row => {
            const chatId = row.chat_id;

            // Sending a message via the bot for each chat ID
            return bot.sendMessage(chatId, "CAN BE ANYTHINGGGGGG get req")
                .catch(error => {
                    console.error(`Error sending message to ${chatId}:`, error);
                });
        });

        // Wait for all messages to be sent
        await Promise.all(messagePromises);

        // Respond after all messages are sent
        res.send("Messages sent to all chat IDs");
    } catch (error) {
        console.error("Error fetching chat IDs or sending messages:", error);
        res.status(500).send("Error fetching chat IDs or sending messages");
    }
});

app.post("/report", async (req, res) => {
	try {
        const { rows } = await sql`SELECT * FROM chatids`;

        const body = req.body;
        const topic = body.topic;
        const payload = body.payload;
        

        const message = topic + "\n\n" + payload;

        // Check if rows exist
        if (rows.length === 0) {
            return res.send("No chat IDs found.");
        }

        // Prepare to send messages
        const messagePromises = rows.map(row => {
            const chatId = row.chat_id;

            // Sending a message via the bot for each chat ID
            return bot.sendMessage(chatId, message)
                .catch(error => {
                    console.error(`Error sending message to ${chatId}:`, error);
                });
        });

        // Wait for all messages to be sent
        await Promise.all(messagePromises);

        // Respond after all messages are sent
        res.send("Messages sent to all chat IDs");
    } catch (error) {
        console.error("Error fetching chat IDs or sending messages:", error);
        res.status(500).send("Error fetching chat IDs or sending messages");
    }
});

app.get("/output", async (req, res) => {
    try {
        // Query to get the current output
        const { rows } = await sql`SELECT output FROM actual_output LIMIT 1`;

        // Check if output is found
        if (rows.length === 0) {
            return res.status(404).send("Output not found.");
        }

        // Respond with the current output
        res.json({ output: rows[0].output });
    } catch (error) {
        console.error("Error fetching current output:", error);
        res.status(500).send("Error fetching current output");
    }
});

app.get("/birdcount", async (req, res) => {
    try {
        // Query to get the current output
        const { rows } = await sql`SELECT output FROM bird_count LIMIT 1`;

        // Check if output is found
        if (rows.length === 0) {
            return res.status(404).send("Output not found.");
        }

        // Respond with the current output
        res.json({ output: rows[0].output });
    } catch (error) {
        console.error("Error fetching current output:", error);
        res.status(500).send("Error fetching current output");
    }
});

app.post("/updatebird", async (req, res) => {
	try {
        const body = req.body;
        const birdCount = body?.birdcount;

        // Update the output field with the payload value
        if (birdCount !== null && birdCount !== undefined) {
            await sql`UPDATE bird_count SET output = ${birdCount}`;
        }
        // Respond after all messages are sent
        res.send("Updated Bird Count");
    } catch (error) {
        res.status(500).send(error);
    }
});

app.post("/updatetray", async (req, res) => {
	try {
        const body = req.body;
        const value = body?.value;

        // Update the output field with the payload value
        if (value !== null && value !== undefined) {
            await sql`UPDATE actual_output SET output = ${value}`;
        }
        // Respond after all messages are sent
        res.send("Updated Tray Count");
    } catch (error) {
        res.status(500).send(error);
    }
});

app.get("/getChat", async (req, res) => {
	try {
		// Assuming `sql` is your database connection
		const { rows } = await sql`SELECT * FROM chatids`;

		res.json(rows);
	} catch (error) {
		console.error("Error retrieving posts:", error);
		res.status(500).send("Internal Server Error");
	}
});

app.get("/add", async (req, res) => {
	try {
		// Assuming `sql` is your database connection
		const { rows } = await sql`INSERT INTO chatids (chat_id) 
VALUES ('chat2233')`;

		res.json(rows);
	} catch (error) {
		console.error("Error retrieving posts:", error);
		res.status(500).send("Internal Server Error");
	}
});

app.post(`/webhook/${token}`, async (req, res) => {
    const update = req.body;

    if (update.message && update.message.text) {
        const chatId = update.message.chat.id;

        try {
            const { rows } = await sql`INSERT INTO chatids (chat_id) 
            VALUES (${chatId})`;
            console.log('Chat ID inserted:', rows);
        } catch (error) {
            console.error('Error inserting chat ID:', error);
            // You can send an error response if needed
            return res.sendStatus(500); // Internal Server Error
        }
    }

    // Respond with a 200 OK
    res.sendStatus(200);
});

//app.listen(3000, () => console.log("Server ready on port 3000."));
if (require.main === module) {
	app.listen(3000, () => console.log("Server ready on port 3000."));
}

module.exports = app;
