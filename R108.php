<?php
$token = {your_toke};
$chat_id = {your_chat_id};
// The member array
$member = array('12345' => 'Alice', '67890' => 'Bob');
if(isset($_GET['UID'])){
        $UID = $_GET['UID'];
        $text = ''; // The text that will send to telegram chatroom.
        if( $member[$UID] ) // if $UID is in member
                $text.= "<b>" . $member[$UID] . '</b> has login <br>' . date("Y-m-d H:i:s");
        else
                $text.= "<b>***Warning***</b><br>This guy with the UID: <b>{$UID}</b> is trying to hack<br>" . date("Y-m-d H:i:s");
		// show in webpage as debugging info
        echo $text;
        $text = str_replace("<br>",'\n',$text);
        $text = rawurlencode($text); // avoid special character
        $text = str_replace("%5Cn",'%0A',$text); // new line should be %0A not \n
		// use HTML mode to send message, also you can use Markdown just modify my code
        $url = "https://api.telegram.org/bot". $token . "/sendMessage?chat_id=". $chat_id . "&parse_mode=HTML&text=";
        $url .= $text;
		// visit the url
        file_get_contents($url);
        }

