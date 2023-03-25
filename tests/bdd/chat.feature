Feature: Chat

    Scenario: Alice chats with Bob
        Given "Alice" connects without a token
        And "Alice" set client name
        Then "Alice" WebSocket receives "1" users

        Given "Bob" connects without a token
        And "Bob" set client name
        Then "Bob" WebSocket receives "2" users
        And "Bob" WebSocket receives rooms update
        And "Alice" WebSocket receives rooms update
        And "Alice" WebSocket receives rooms update
        And "Alice" WebSocket receives added "Bob" user

        Given "Bob" posts chat message "'hello alice'"
        Then "Alice" WebSocket receives chat message "hello alice" from "Bob"
        Then "Bob" WebSocket receives chat message "hello alice" from "Bob"
        
        Given "Alice" posts chat message "'hello bob'"
        Then "Bob" WebSocket receives chat message "hello bob" from "Alice"
        Then "Alice" WebSocket receives chat message "hello bob" from "Alice"

        Then "Alice" closes WebSocket
        Then "Bob" closes WebSocket
