Feature: Login/Logout

    Scenario: WebSocket Session Login
        Given "Alice" connects without a token
        Then "Alice" WebSocket receives "1" users
        And "Alice" closes WebSocket
    
    Scenario: Login as a audience user
        Given "Alice" connects without a token
        And "Alice" set client name
        And "Alice" upload a avatar
        Then "Alice" authorized as audience user

    Scenario: WebSocket Close
        Given "Alice" connects without a token
        And "Alice" set client name
        Then "Alice" WebSocket receives "1" users
        Given "Bob" connects without a token
        Then "Bob" WebSocket receives "2" users
        And "Alice" closes WebSocket
        And "Bob" WebSocket receives "Alice" delete
        And "Bob" closes WebSocket
    
    Scenario: User Logout 
        Given "Alice" connects without a token
        And "Alice" set client name
        Then "Alice" WebSocket receives "1" users
        Given "Bob" connects without a token
        And "Bob" set client name
        Then "Bob" WebSocket receives "2" users
        And "Alice" WebSocket receives added "Bob" user
        And "Alice" logouts
        And "Bob" WebSocket receives "Alice" delete
        And "Bob" closes WebSocket
