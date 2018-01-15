<?php
$token = {your_toke};
$chat_id = {your_chat_id};
$dsn = "mysql:host=localhost;dbname={DB_name}";
$db = new PDO($dsn, {user}, {pwd}); 

function show($text){
    // show in webpage as debugging info
    echo $text;
    global $token, $chat_id;
    $text = str_replace("<br>",'\n',$text);
    $text = rawurlencode($text); // avoid special character
    $text = str_replace("%5Cn",'%0A',$text); // new line should be %0A not \n
    // use HTML mode to send message, also you can use Markdown just modify my code
    $url = "https://api.telegram.org/bot". $token . "/sendMessage?chat_id=". $chat_id . "&parse_mode=HTML&text=";
    $url .= $text;
    // visit the url
    file_get_contents($url);
}

if(isset($_GET['UID'])){
        $UID = $_GET['UID'];
        $user = $_GET['user'];
        $text = ''; // The text that will send to telegram chatroom.
        if($user != ''){  // if this request is for register
            $sql = "select 1 from embedded where UID = '". $UID ."';";
            if($db->query($sql)->rowCount()){  // if this UID has been registered
                $text = "Warning, this user: ". $user ." has already registered.";                
            }
            else{  // write into database
                $sql = "INSERT INTO `embedded`(`UID`, `user`) VALUES ('$UID','$user')";
                $db->exec($sql);
                $text = "The user:". $user ."registered successfully.";
            }
        }
        else{  // this request is for unlock
            $sql = "select * from embedded where UID = '". $UID ."';";
            $result = $db->query($sql);
            if( $result->rowCount() > 0 ){ // if $UID is in member
                foreach($result as $row){
                    $text.= "<b>" . $row["user"] . '</b> has login <br>' . date("Y-m-d H:i:s");
                } 
            }
            else
                $text.= "<b>***Warning***</b><br>This guy with the UID: <b>{$UID}</b> is trying to hack<br>" . date("Y-m-d H:i:s");
        }
        show($text);
}
